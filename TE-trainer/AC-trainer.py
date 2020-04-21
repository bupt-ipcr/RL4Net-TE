#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import argparse
import time
import json
import numpy as np
from pathlib import Path
from pprint import pprint
import random
import re
import sys
from typing import List
from pyns3 import ns3env
from torch.utils.tensorboard import SummaryWriter
from utils import create_adjacencymatrix_from_map, create_tms, save_tms, get_reachablematrix

# 确保跨文件夹的搜索成功
cwd = Path.cwd()
print(f'cwd: {cwd}')
# ！注意：rlagent_path 应当是RLAgent module 的父文件夹，如'/RL4Net'
rlagent_path = None
if 'RLAgent' in (p.name for p in cwd.iterdir()):
    rlagent_path = str(cwd)
else:
    for parent in cwd.parents:
        if 'RLAgent' in (p.name for p in parent.iterdir()):
            rlagent_path = str(parent)
            break   # break to reduce searching
    
if rlagent_path is not None and rlagent_path not in sys.path:
    print(f'rlagent path is {rlagent_path} and is not in sys.path')
    sys.path.append(rlagent_path)
    
from RLAgent import DDPG

parser = argparse.ArgumentParser(description='Start simulation script on/off')
parser.add_argument('--start',
                    type=int,
                    default=1,
                    help='Start ns-3 simulation script 0/1, Default: 1')
parser.add_argument('--routing',
                    type=str,
                    default='rl',
                    help='Routing method, Default: rl routing')

args = parser.parse_args()
startSim = bool(args.start)

#  环境名
simScripteName = 'udp-fm'
# 超参
MAX_EPISODE = 1000
MAX_STEP = 100

simTime = 5 * 2  # seconds
stepTime = 0.5  # seconds
seed = 0
debug = False
simArgs = {"--maxStep": MAX_STEP,
           "--routingMethod": args.routing,
           "--adjacencyMatrix": '''[0,1,1,1,-1,0,-1,1,-1,-1,0,1,-1,-1,-1,0]''',
           "--trafficMatrix": '''[{"/src/":0,"/dst/":3,"/rate/":5}]'''
           }


def create_agent(n_states: int, n_actions: int, tms_size: int):
    """
    指定超参，创建DDPG agent
    @param n_states: 有多少个state
    @param n_actions: 有多少个action
    @param tms_size: 有多少个业务需求
    """
    ACTOR_LEARNING_RATE, CRITIC_LEARNING_RATE = 0.0001, 0.001   # AC的学习率
    max_mem, min_mem, batch_size = 100 if tms_size < 20 else 1000, 64, 32   # 约定经验回放池参数

    agent = DDPG(n_states, n_actions, MAX_MEM=max_mem, MIN_MEM=min_mem, BATCH_SIZE=batch_size, a_bound=1)
    return agent


def rl_loop(agent, need_load=True):
    """
    强化学习的主循环
    """
    if need_load:
        START_EPISODE = agent.load()
    else:
        START_EPISODE = 0
    try:
        summary_writer = agent.get_summary_writer()
        for e in range(START_EPISODE, MAX_EPISODE):
            cum_reward = 0
            print("Start episode: ", e, flush=True)
            cur_index = random.randint(0, len(traffic_matrix_state_list) - 1)
            cur_state = traffic_matrix_state_list[cur_index]   # 随机选取一个tm作为当前state
            # 为这个特定的cur_state重建环境
            simArgs.update({
                "--trafficMatrix": traffic_matrix_str_list[cur_index]
            })
            env = ns3env.Ns3Env(port=5555, stepTime=stepTime, startSim=startSim, simSeed=seed, simArgs=simArgs, debug=debug, simScriptName=simScripteName)
            cur_state = env.reset()
            for s in range(MAX_STEP):
                print("Step: ", s, flush=True)

                # 选取动作
                noise_decay_rate = max((100 - e) / 100, 0.01)
                action = agent.get_action_noise(cur_state, rate=noise_decay_rate)[0]

                next_state, reward, done, info = env.step(action)

                cum_reward += reward
                # print('cur_state_str: ', traffic_matrix_str_list[cur_index])
                info = {
                    "cur_state": list(cur_state), "action": list(action),
                    "next_state": list(next_state), "reward": reward, "done": done
                }
                print(json.dumps(info))
                
                agent.add_step(np.array(cur_state), action, reward, done, np.array(next_state))  # 添加到经验回放池
                
                cur_state = next_state

                if done:
                    break
                # 训练
                agent.learn_batch()
            summary_writer.add_scalar('cum_reward', cum_reward, e)
            agent.save(e)   # 保存网络参数
            if env:
                env.close()
    except KeyboardInterrupt:
        print('正在保存网络参数，请不要退出\r', end='')
        agent.save(e)
        print("Ctrl-C -> Exit                   ")
    finally:
        if summary_writer:
            summary_writer.close()
        if env:
            env.close()
        print("Done!")


if __name__ == '__main__':
    # adjacencyMatrix = create_adjacencymatrix_from_map('http://www.topology-zoo.org/files/Ans.gml')
    adjacencyMatrix = [0, 1, 1, 1, -1, 0, -1, 1, -1, -1, 0, 1, -1, -1, -1, 0]  # 使用四节点测试
    simArgs.update({
        "--adjacencyMatrix": '[' + ','.join([str(m) for m in adjacencyMatrix]) + ']'
    })
    # traffic_matrix_str_list, traffic_matrix_state_list = create_tms(adjacencyMatrix, total_rate=4, max_tms=1)
    # TODO 将traffic_matrix抽象一层
    traffic_matrix_str_list = ['''[{/src/:0,/dst/:3,/rate/:5}]''']
    traffic_matrix_state_list = [[0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]]
    save_tms(traffic_matrix_str_list)
    print(f'create an agent as inputs {len(adjacencyMatrix),}, outputs {adjacencyMatrix.count(1)}.')
    agent = create_agent(len(adjacencyMatrix), adjacencyMatrix.count(1), len(traffic_matrix_state_list))

    # 创建NN
    rl_loop(agent)
