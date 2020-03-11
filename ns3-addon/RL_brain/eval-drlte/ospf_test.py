#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 1970-01-01 08:00
@edit time: 2019-12-20 11:17
@file: /scratch/eval-drlte/ospf_test.py
"""


import argparse
import random
import numpy as np
import json
from ns3gym import ns3env
import re


baseSimArgs = {"--simTime": 10,
               "--routingMethod": 'ospf',
               "--adjacencyMatrix": '''[0,1,1,1,1,0,1,1,-1,-1,0,1,-1,-1,1,0]''',
               "--trafficMatrix": '''[{"/src/":0,"/dst/":3,"/rate/":1}]'''
               }


def get_ospf_reward(tm, simArgs):
    """
    获取OSPF路由规则下的reward。注意reward=-delay，单位是ms。
    @param tm: 用于测试的业务TrafficMatrix
    @param simArgs: 包含启动一个ns3脚本需要的其他参数
    """

    simArgs.update({
        "--trafficMatrix": tm
    })
    print('  start a sim')
    env = ns3env.Ns3Env(port=5555, stepTime=0.5, startSim=True, simSeed=0, simArgs=simArgs, debug=False)
    obs = env.reset()
    ac_space = env.action_space
    action = [random.random() * 10 for _ in range(ac_space.shape[0])]
    obs, reward, done, info = env.step(action)
    print("  ---reward, done, info: ", reward/1000000, done, info)
    if env:
        env.close()
    return reward / 1000000


def get_tms_from_file(file_path='tms.save'):
    """
    从保存的文件中读取所有TrafficMatrix。
    在drlte_torch运行的时候会执行保存函数，将创建的所有TMs保存到文件中，默认位置是 ./tms.save
    drlte_torch运行时同时会输出到nohup.out文件中，其中state（即TM）只保存其序号
    通过读取文件就可以知道序号对应的TM具体内容
    @param file_path: 要读取的文件的序号
    @return: 返回一个List[str]类型的states，按顺序记录了所有的state（即TM）信息。
    """
    states = []
    with open(file_path, 'r') as f:
        for line in f.readlines():
            states.append(line.split('-')[1].strip())
    return states


def get_ospf_rewards(simArgs, tms=None):
    """
    给定一组tms，获取这组tms在OSPF路由规则下对应的所有reward
    @param tms: 需要仿真的所有TMs
    @return: List[float] 返回每个TM对应的reward构成的列表
    """
    if not tms:
        tms = get_tms_from_file()
    reward_list = []
    for tm in tms:
        simArgs.update({
            "--trafficMatrix": tm
        })
        reward = get_ospf_reward(tm, simArgs)
        reward_list.append(reward)
    return reward_list


def get_baseline(adjacencyMatrix):
    """
    用于获取baseline的接口
    """
    simArgs = baseSimArgs
    simArgs.update({
        "--adjacencyMatrix": '[' + ','.join([str(m) for m in adjacencyMatrix]) + ']'
    })
    return get_ospf_rewards(simArgs)


if __name__ == '__main__':
    print(get_tms_from_file())
