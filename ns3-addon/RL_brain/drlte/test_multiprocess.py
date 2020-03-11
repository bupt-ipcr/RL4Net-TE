#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 1970-01-01 08:00
@edit time: 2019-12-23 12:05
@desc
"""


from multiprocessing import Process
from multiprocessing import Queue
from multiprocessing import Pipe
import time
import re
import sys
import os
from drlte_torch import create_agent
from ns3gym import ns3env
from torch.utils.tensorboard import SummaryWriter
from utils import create_adjacencymatrix_from_map, create_tms, save_tms, get_reachablematrix, create_tms_gravity
import random
import numpy as np
import subprocess


simArgs = {"--simTime": 10,
           "--routingMethod": 'rl',
           "--adjacencyMatrix": '''[0,1,1,1,1,0,1,1,-1,-1,0,1,-1,-1,1,0]''',
           "--trafficMatrix": '''[{"/src/":0,"/dst/":3,"/rate/":1}]'''
           }


class RLBrain:
    def __init__(self, agent, q, p_dict, max_episodes, max_steps, need_load=True):
        """初始化类"""
        self.agent = agent
        self.queue, self.pipe_dict = q, p_dict
        self.max_episodes, self.max_steps = max_episodes, max_steps
        self.noise_decay_max = 30
        self.need_load = need_load
        self.heartbeat_dict, self.wscript_dict = {}, {}     # 记录心跳包内容并监控
        self.summary_writer = self.agent.get_summary_writer()    # 获取summary_writer
        self.act_writer = open('train.act', 'w')
        self.monitor_writer = open('train.moi', 'w')

    def _load_model(self):
        """加载模型参数"""
        if self.need_load:
            return self.agent.load()
        else:
            return 0

    def train(self, record):
        """
        接收到一条仿真记录，执行：
        打印相关信息，经验回放池增加一步，训练
        @param record: dict, 包含训练全部信息的仿真记录
        """
        print("reward: ", record['reward'], ", method: ", record['info'], ", cur_state_index: ", record['cur_index'])
        self.agent.add_step(record['cur_state'], record['action'], record['reward'], record['done'], record['next_state'])
        self.agent.learn_batch()

    def act(self, pindex, state, e):
        """
        接收到一条获取动作的请求，返回动作
        @param state: 请求获取动作的state
        @param e: 当前episode，用于计算噪声的rate
        """
        action = self.agent.get_action_noise(state, rate=(max(self.noise_decay_max - e / self.noise_decay_max, 0.01)))[0] 
        print(f'pindex: {pindex}, action: {action}', file=self.act_writer, flush=True)
        pipe = self.pipe_dict[pindex]
        pipe.send(action)

    def count_down(self, pindex, waf_string):
        """
        心跳监控
        每当有一个请求动作的包到来，同时也意味着子进程开启了一个新的ns3Env
        所以我们除了返回动作意外，还需要考虑ns3Env启动失败导致进程卡死的问题
        为了记录该进程启动ns3脚本的参数，我们将整个waf_string记录到一个dict中，与其pindex一一对应
        同时为了判断一个进程是不是卡死，我们使用倒计时机制：
        每当有一个新的心跳到来，将该心跳包的计数重置为32，然后将所有心跳包计数减1
        如果有一个进程在32个计数内没有新的心跳到来，则认为它可能卡住了，使用指令检测ns3脚本是否启动，若未启动则帮其启动并记录
        TODO 1. 将32参数化 2. 考虑32个心跳内所有进程都卡死的情况
        """
        self.heartbeat_dict[pindex] = 32
        self.wscript_dict[pindex] = waf_string
        for p in self.heartbeat_dict:
            self.heartbeat_dict[p] -= 1
            # 如果出现比0小的，有可能对应的ns3进程挂了，需要重启
            if self.heartbeat_dict[p] <= 0:
                # 再次确认是否有对应的ns3进程
                ret = os.popen('ps -ef | grep 5001 | grep adj | wc -l')
                p_count = int(ret.read().strip())
                if p_count > 1:  # 如果>1，则在grep之外有一个包含adj(即waf进程) 端口又是5001的进程
                    continue
                else:   # 说明确实没有对应进程，需要重新启动
                    wafString = self.wscript_dict[p]    # 找到对应的waf脚本
                    subprocess.Popen(wafString, shell=True, stdout=None, stderr=None)   # 用subprocess.Popen()的方式启动
                    print(f'restart ns3 with wscript {wafString}', file=self.monitor_writer, flush=True)  # 并记录这次重启

    def main_loop(self):
        # 加载模型
        start_episode = self._load_model()
        # 开启主循环
        for e in range(start_episode, self.max_episodes):
            cum_reward = 0  # 每个episode重置cum_reward
            print("Start episode: ", e, flush=True)
            s = 0
            while s < self.max_steps:
                event = self.queue.get()
                # 如果拿到的是一个进程的put请求，即增加一个step，则训练。只有训练被计入step，获取动作是不被计入step的
                if event['method'] == 'put':
                    print("Step: ", s, flush=True)  # 打印step信息
                    self.train(event['record'])     # 执行训练流程
                    cum_reward += event['record']['reward']  # 记录cum_reward
                    s += 1
                # 如果拿到的是一个进程的get请求，则获取action并传给进程。同时记录该心跳包用于监控
                if event['method'] == 'get':
                    self.act(event['pindex'], event['state'], e)   # 执行动作流程
                    self.count_down(event['pindex'], event['wafString'])  # 监控各包状况
            self.summary_writer.add_scalar('cum_reward', cum_reward, e)  # 记录cum_reward
            self.agent.save(e)   # 保存网络参数


def ns3_process(pindex, q, p, tmstrs, tmstates):
    # 约定port
    port = 5001+pindex
    while True:
        cur_index = random.randint(0, len(tmstates) - 1)
        cur_state = tmstates[cur_index]   # 随机选取一个tm作为当前state
        # 为这个特定的cur_state重建环境
        simArgs.update({
            "--trafficMatrix": tmstrs[cur_index]
        })
        env = ns3env.Ns3Env(stepTime=1, startSim=1, simSeed=0, simArgs=simArgs, debug=False, port=port)
        obs = env.reset()

        # 获取动作
        simScriptName = os.path.basename(os.getcwd())           # 获取scriptname
        simSeed = np.random.randint(0, np.iinfo(np.uint32).max)  # 获取simSeed
        wafString = f'''/root/drlte-ns3gym/waf --run "{simScriptName} --openGymPort={port} --simSeed={simSeed}'''
        for k, v in simArgs.items():
            wafString += f" {k}={v}"
        q.put({
            'pindex': pindex,
            'method': 'get',
            'state': cur_state,
            'wafString':  wafString,
        })
        action = p.recv()

        _, reward, done, info = env.step(action)
        reward = reward / 1000000   # 将reward限制在合理的范围内
        next_state = tmstates[random.randint(0, len(tmstates) - 1)]   # 随机选取一个tm作为下一state
        env.close()
        q.put({
            'pindex': pindex,
            'method': 'put',
            'record': {
                'cur_index': cur_index, 'action': action, 'next_state': np.array(next_state),
                'cur_state': np.array(cur_state), 'reward': reward, 'done': done, 'info': info
            }
        })


def start_drlte():
    MAX_EPISODES = 1000
    MAX_STEPS = 1000
    adjacencyMatrix = create_adjacencymatrix_from_map('http://www.topology-zoo.org/files/Ans.gml')
    # adjacencyMatrix = [0, 1, 1, 1, 1, 0, 1, 1, -1, -1, 0, 1, -1, -1, 1, 0]  # 使用四节点测试
    simArgs.update({
        "--adjacencyMatrix": '[' + ','.join([str(m) for m in adjacencyMatrix]) + ']'
    })
    traffic_matrix_str_list, traffic_matrix_state_list = create_tms_gravity(adjacencyMatrix, total_rate=15, max_tms=50)
    # 使用特定tm训练
    # traffic_matrix_str_list = ['''[{/src/:1,/dst/:2,/rate/:1.0828932408423846},{/src/:2,/dst/:3,/rate/:1.0967429093466428},{/src/:0,/dst/:1,/rate/:1.6143097493911085},{/src/:3,/dst/:2,/rate/:0.921152445613088}]''']
    # traffic_matrix_state_list = [[0,1.6143097493911085,0,0, 0,0,1.0828932408423846,0, 0,0,0,1.0967429093466428, 0,0,0.921152445613088,0]]
    save_tms(traffic_matrix_str_list)
    print(f'create an agent as inputs {len(adjacencyMatrix),}, outputs {adjacencyMatrix.count(1)}.')
    ddpg = create_agent(len(adjacencyMatrix), adjacencyMatrix.count(1), len(traffic_matrix_state_list))

    queue = Queue()

    main_p_pipes = {}
    ns3_ps = []
    for i in range(8):
        main_pipe, ns3_pipe = Pipe()
        p = Process(target=ns3_process, args=(i, queue, ns3_pipe, traffic_matrix_str_list, traffic_matrix_state_list))
        ns3_ps.append(p)
        main_p_pipes[i] = main_pipe
    time.sleep(1)
    for p in ns3_ps:
        time.sleep(1)
        p.start()
    # main_p = Process(target=train_process, args=(ddpg, queue, main_p_pipes, MAX_EPISODES, MAX_STEPS))
    brain = RLBrain(ddpg, queue, main_p_pipes, MAX_EPISODES, MAX_STEPS)
    brain.main_loop()


def brain_test():
    # 测试监控和输出
    ddpg = create_agent(16, 8, 16)
    queue = Queue()
    pipe1_main, pipe1_ns3 = Pipe()
    pipe2_main, pipe2_ns3 = Pipe()
    brain = RLBrain(ddpg, queue, {1: pipe1_main, 2: pipe2_main}, 1000, 1000)
    brain.count_down(1, 'waf1')
    for i in range(30):
        brain.act(1, [123], 2)
        brain.count_down(2, f'waf2_{i}')
    brain.count_down(2, 'adsf')
    brain.count_down(2, 'adsf')
    brain.count_down(2, 'adsf')
    brain.count_down(2, 'adsf')
    brain.count_down(2, 'adsf')


if __name__ == '__main__':
    start_drlte()
