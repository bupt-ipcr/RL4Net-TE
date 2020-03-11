#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 1970-01-01 08:00
@edit time: 2020-01-07 15:46
@file: /simple_test.py
"""


import argparse
import random
import numpy as np
import json
from ns3gym import ns3env
from ddpg import DDPG
import numpy as np

__author__ = "Piotr Gawlowicz"
__copyright__ = "Copyright (c) 2018, Technische Universität Berlin"
__version__ = "0.1.0"
__email__ = "gawlowicz@tkn.tu-berlin.de"


def get_action():
    adjacencyMatrix = [0, 1, 1, 1, 1, 0, 1, 1, -1, -1, 0, 1, -1, -1, 1, 0]  # 使用四节点测试
    agent = DDPG(len(adjacencyMatrix), adjacencyMatrix.count(1), a_bound=adjacencyMatrix.count(1) * 2)
    agent.load()
    state_str = '''[{/src/:1,/dst/:2,/rate/:3.0828932408423846}]'''
    state_list = json.loads(state_str.replace('/', '"'))
    state = np.zeros(16)
    for s in state_list:
        state[s['src'] * 4 + s['dst']] = s['rate']
    return agent.get_action(state), agent.get_action_noise(state, rate=1)


def get_rl_reward():
    simArgs = {"--simTime": 10,
               "--routingMethod": 'ospf',
               "--adjacencyMatrix": '''[0,1,1,1,1,0,1,1,-1,-1,0,1,-1,-1,1,0]''',
               "--trafficMatrix": '''[{/src/:1,/dst/:2,/rate/:4.636}]'''
               }

    print('  start a sim')
    env = ns3env.Ns3Env(port=5555, stepTime=0.5, startSim=True, simSeed=0, simArgs=simArgs, debug=True)
    obs = env.reset()
    ac_space = env.action_space
    # action, action_niose = get_action()
    # action = action[0].clip(min=1)
    action = [1,1,1,8,1,1,1,8]
    print('action', action)
    obs, reward, done, info = env.step(action)
    print("  ---reward, done, info: ", reward/1000000, done, info)
    if env:
        env.close()
    return reward / 1000000

if __name__ == '__main__':
    print(get_action())
    