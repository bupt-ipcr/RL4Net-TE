#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-12-10 20:44
@edit time: 2020-01-07 15:55
@file: visulize.py
@desc: 可视化训练数据
"""


from ddpg import OUProcess
import matplotlib.pyplot as plt
from tqdm import trange, tqdm
from pprint import pprint
import numpy as np
import re
from ospf_test import get_baseline
from utils import create_adjacencymatrix_from_map
import argparse
import json


parser = argparse.ArgumentParser(description='绘制RL时延和OSPF时延时的参数')
parser.add_argument('--baseline', '-b', default=False, action='store_true', help='是否需要绘制OSPF的baseline')
parser.add_argument('--simple', '-s', default=False, action='store_true', help='是否使用四节点简单模型')
parser.add_argument('--steps', type=int, default=100, help='一个episode有多少个step')
args = parser.parse_args()


def ou_param(m_sigma=10, m_theta=0.16):
    """
    可视化OU过程产生的分布情况
    @param m_sigma: OU过程的sigma参数
    @param m_theta: OU过程的theta参数
    """
    xs, ys = [], []
    noise = OUProcess(1, sigma=m_sigma, theta=m_theta)
    for i in trange(100000):
        xs.append(i)
        ys.append(noise.noise())
    d = np.array(ys)
    plt.hist(d, bins=1000)
    plt.savefig('oufog.png')
    plt.close()


def get_loginfo_from_file(file_path='train.log'):
    """
    从log信息中读取信息，包括step, reward, method, state
    @param file_path: 要读取的文件位置，默认为train.log
    @return: List[Dict] 将一组信息保存为Dict，最后放在一个list中返回
    """
    infos, index = [], 0
    info_re = re.compile(r'reward:  (-[\d.]+?) , method:  ([\w]+?) , cur_state_index:  (\d+)')
    with open(file_path) as f:
        for line in f.readlines():
            info = info_re.search(line)
            if info:
                infos.append({
                    'step': index,
                    'reward': info[1],
                    'method': info[2],
                    'state': info[3]
                })
                index += 1
    return infos


def draw_delay(adjacencyMatrix, steps, baseline=False):
    """
    绘制时延图。其中：
    RL方法的时延通过log文件读取并计算，其简要逻辑为
      通过读取log文件能得到每一个step的reward，即 - delay
        还能得到一共有哪些state（用index指代）
      通过指定steps可以将上述所有step分成段（通常按episode分段）
      对每段steps，统计每个state的平均时延，再对state求平均，就得到：
        在state取到的概率相等的假设下平均一个state的时延
    OSPF的方法通过ospf_test中暴露的接口计算，其简要逻辑为
      读取tms.save中保存的tms，将tms应用到OSPF上获取全部时延。计算时延的均值。
    @param adjacencyMatrix: 不同的邻接矩阵会影响baseline，所以需要传入邻接矩阵
    @param steps: 绘制平均时延的时候要约定是“多少step平均一次”，即steps
    @param baseline: 是否要绘制OSPF作为baseline
    """
    # 从log文件中读到的是step的列表
    step_list = get_loginfo_from_file()
    # 记录有多少个state，这是计算delay的依据
    # 注意在state过长的时候，这里存储的是state的index作为代替
    states = set([step['state'] for step in step_list])
    drl_delays = []
    for i in range(len(step_list)//steps):
        # 按照steps将记录分段读取
        step_batch = step_list[i*steps:i*steps+steps:]
        cum_delay = 0
        # 遍历所有状态，计算每个状态平均的delay
        for state in states:
            # 对某个状态获取全部的delay（即 - reward）
            state_delays = [- float(l['reward']) for l in step_batch if l['state'] == state]
            avg_delay = np.mean(state_delays) if state_delays else 0
            cum_delay += avg_delay
        # drl_delay只要计算 delay per state 即可，不需要乘steps
        drl_delay = cum_delay / len(states)
        drl_delays.append(drl_delay)
    plt.title('avg delay')
    plt.plot(drl_delays)
    plt.xlabel('step')
    plt.ylabel('delay / ms')
    plt.legend(['drl delay'])
    plt.savefig('drl_delay.png')

    if baseline:
        # 将state依次用ospf的方式运行获得reward
        rewards = get_baseline(adjacencyMatrix)
        # 同理，ospf delay 也只需要计算到按state平均的值即可，不需要乘steps
        ospf_delay = - np.mean(rewards)
        # 绘制ospf_delay作为baseline
        plt.plot(np.full(len(drl_delays), ospf_delay))
        plt.legend(['drl delay', 'ospf delay'])
        plt.savefig('drl_ospf_delay.png')
    plt.close()


def draw_ospf_load():
    """绘制ospf的load"""
    plt.title('ospf load')
    with open('delay_info.json') as f:
        infos = json.load(f)
    info_list = [(info['rate'], info['avg_delay'], info['delay_details']) for info in infos]
    rates, avgs, details = zip(*info_list)
    plt.plot(rates, avgs)
    plt.plot(rates, [np.median(detail) for detail in details])
    plt.plot(rates, [np.max(detail) for detail in details])
    plt.legend(['avg', 'median', 'max'])
    plt.xlabel('total rate / Mbps')
    plt.ylabel('time / ms')
    plt.savefig('figs/overview.png')
    plt.close()
    for size in range(5):
        for index, detail in enumerate(details):
            if not size*10+1 <= index <= size*10+10:
                continue
            detail.sort()
            plt.plot(detail)
        plt.legend(range(size*10+1, size*10+10+1), loc=2)
        plt.xlabel('tm index')
        plt.ylabel('time / ms')
        plt.savefig(f'figs/detail_{size}.png')
        plt.close()


def draw_trend():
    lines = get_loginfo_from_file()
    states = set([line['state'] for line in lines])
    state_container = {s: [] for s in states}
    state_records = {s: [] for s in states}
    state_steps = {s: [] for s in states}

    for line in lines:
        state_container[line['state']].append({'reward': float(line['reward']), 'step': int(line['step'])})
    for state, records in state_container.items():
        record_list, step_list = [], []
        records.sort(key=lambda x: x['step'])
        for record in records:
            record_list.append(record['reward'])
            step_list.append(record['step'])
        state_records[state] = np.array(record_list)
        state_steps[state] = np.array(step_list)
    i = 0
    for state, records in tqdm(state_records.items(), desc='ploting...'):
        plt.title(state)
        # x = np.arange(len(records))
        # plt.plot(x, records)
        plt.plot(state_steps[state], records)
        plt.savefig(f'fig{i}.png')
        plt.close()
        i += 1


if __name__ == '__main__':
    if args.simple:
        # 如果制定了使用简单模型，则使用四节点测试
        adj_matrix = [0, 1, 1, 1, 1, 0, 1, 1, -1, -1, 0, 1, -1, -1, 1, 0]  
    else:
        adj_matrix = create_adjacencymatrix_from_map('http://www.topology-zoo.org/files/Ans.gml')
    draw_delay(adjacencyMatrix=adj_matrix, steps=args.steps, baseline=args.baseline)
