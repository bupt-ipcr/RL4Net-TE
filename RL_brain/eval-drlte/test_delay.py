#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 1970-01-01 08:00
@edit time: 2020-03-03 16:59
@FilePath: /RL_brain/eval-drlte/test_delay.py
"""
import time
import argparse
import random
import numpy as np
import json
from ns3gym import ns3env
import re
from utils import create_adjacencymatrix_from_map, get_reachablematrix, create_tms

parser = argparse.ArgumentParser(description='参数')
parser.add_argument('--routing', default='rl', type=str, help='路由规则')
parser.add_argument('--rate', type=float, default=1, help='总速率')
args = parser.parse_args()

baseSimArgs = {"--maxStep": 100,
               "--routingMethod": 'rl',
               "--adjacencyMatrix": '''[0,1,1,1,1,0,1,1,-1,-1,0,1,-1,-1,1,0]''',
               "--trafficMatrix": '''[{"/src/":0,"/dst/":3,"/rate/":1}]'''
               }
baseSimArgs.update({
    "--routingMethod": args.routing
})


def get_reward(tm, simArgs):
    """
    获取OSPF路由规则下的reward。注意reward=-delay，单位是ms。
    @param tm: 用于测试的业务TrafficMatrix
    @param simArgs: 包含启动一个ns3脚本需要的其他参数
    """

    simArgs.update({
        "--trafficMatrix": tm
    })
    print('  start a sim')
    # 从创建env开始计时
    cur = time.time()
    env = ns3env.Ns3Env(port=5555, startSim=True, simSeed=0, simArgs=simArgs, debug=False, simScriptName='udp-fm')
    obs = env.reset()
    ac_space = env.action_space
    action = [random.random() * 10 for _ in range(ac_space.shape[0])]
    obs, reward, done, info = env.step(action)
    # 到返回结果为止停止
    print('exec time: ', time.time() - cur)
    print("  ---reward, done, actioo, info: ", reward, done, action, info)
    if env:
        env.close()
    return reward 
    

if __name__ == '__main__':
    rate = args.rate
    # adjacencyMatrix = create_adjacencymatrix_from_map('http://www.topology-zoo.org/files/Ans.gml')
    adjacencyMatrix = [0, 1, 1, 1, -1, 0, -1, 1, -1, -1, 0, 1, -1, -1, -1, 0]
    baseSimArgs.update({
        "--adjacencyMatrix": '[' + ','.join([str(m) for m in adjacencyMatrix]) + ']'
    })
    tms = []
    with open('tms.save', 'r') as f:
        for line in f.readlines():
            tms.append(line.split('-')[1])
    rewards = []
    for tm in tms:
        reward = get_reward(tm, baseSimArgs)
        print(reward)
        rewards.append(reward)
    print(rewards)
    rewards = np.array(rewards)
    rewards = -rewards
    print(np.mean(rewards))

# 4 nodes test 1
# [-2.219264, -4.401656, -2.285063, -1.986135, -2.228562, -1.926141, -2.377971, -1.950515]
# 2.5028955

# 4 nodes test 2
# [-2.286552, -3.283066, -2.508975, -1.967852, -2.241967, -2.086495, -3.090523, -1.864111]
# 2.416192625

# 4 nodes test 3
# [-2.167948, -3.836301, -2.36045, -2.24292, -2.729594, -2.062802, -2.737544, -1.885605]
# 2.421913375

# 18 nodes test
# [-290.306944, -14.981267, -132.659776, -91.43368, -164.794064, -36.445272, -220.383344, -262.793552, -211.131136, -171.21456, -143.667872, -166.306944, -149.204624, -147.360592, -123.584032, -119.319928, -176.480352, -169.321824, -138.580944, -284.603968, -195.53264, -241.449072, -115.189888, -73.99716, -72.11768, -174.251552, -256.082944, -2.822094, -195.377776, -210.629072, -163.756656, -142.016672, -156.064288, -97.320976, -133.65364, -88.036624, -218.874864, -82.705184, -218.163568, -171.870576, -135.09696, -216.999488, -67.948096, -244.679568, -86.87256, -165.622176, -119.342608, -46.575592, -181.66936, -92.672152, -202.129104, -188.951552, -257.872912, -157.627824, -294.823136, -127.193992, -247.299536, -217.987536, -86.811576, -196.40504, -142.831552, -305.733568, -107.574704, -158.976352, -74.699904, -166.064368, -177.53776, -205.472592, -143.178656, -133.347336, -205.201056, -284.759232, -156.446512, -122.700832, -237.111968]
# 162.40939681333333