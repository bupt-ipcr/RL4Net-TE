#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 1970-01-01 08:00
@edit time: 2019-12-20 11:21
@desc: 测试OSPF在大拓扑下的负载问题
共有25条信道，每条信道容量是5Mbps，理论的network capacity=125(Mbps)
从1Mbps开始，每1Mbps测一次，共测试125次，记录网络时延信息
"""
from utils import create_adjacencymatrix_from_map, create_tms, save_tms
from ospf_test import get_ospf_rewards
import numpy as np
import json
from ns3gym import ns3env
import random
import os


def record_all_delay(adjacencyMatrix, file_path='delay_info.json'):
    # 定义范围
    rate_step = 1
    rate_max = 125
    # 设置simArgs
    simArgs = {"--simTime": 10,
               "--routingMethod": 'ospf',
               "--adjacencyMatrix": '[' + ','.join([str(m) for m in adjacencyMatrix]) + ']',
               "--trafficMatrix": '''[{"/src/":0,"/dst/":3,"/rate/":1}]'''
               }
    # 读取已有文件
    if os.path.exists(file_path):
        with open(file_path, 'r') as f:
            rate_info_list = json.load(f)
    else:
        rate_info_list = []
    start_rate = rate_step * (len(rate_info_list) + 1)
    for rate in range(start_rate, rate_max + rate_step, rate_step):     # 从start_rate开始以rate_step测试，直到rate_max（range是左闭右开的区间）
        print('rate: ', rate)
        # 对每个速率（即对应的流量强度），创建全部TMs并计算对应的时延
        tms, _ = create_tms(adjacencyMatrix, total_rate=rate, need_print=False)
        reward_list = get_ospf_rewards(simArgs, tms=tms)
        delay_list = [- reward for reward in reward_list]
        avg_delay = np.mean(delay_list)
        detail_str = ', \n'.join([f'\t{index}: {delay}' for index, delay in enumerate(delay_list)])
        # 记录这个rate对应的信息
        rate_info = {
            'rate': rate,
            'avg_delay': avg_delay,
            'delay_details': [
                delay for delay in delay_list
            ]
        }
        # 添加到list中
        rate_info_list.append(rate_info)
        # 整个列表存储到文件中
        with open('delay_info.json', 'w') as f:
            json.dump(rate_info_list, f)


if __name__ == '__main__':
    adjacencyMatrix = create_adjacencymatrix_from_map('http://www.topology-zoo.org/files/Ans.gml')
    record_all_delay(adjacencyMatrix)
