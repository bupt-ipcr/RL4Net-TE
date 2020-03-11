#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-12-12 15:52
@edit time: 2019-12-22 22:25
@file: /root/drlte-ns3gym/scratch/drlte/utils.py
"""

import requests
import re
import random
import numpy as np
from pprint import pprint


def create_adjacencymatrix_from_map(url: str = 'http://www.topology-zoo.org/files/Cernet.gml'):
    """从给定的url中读取.gml文件，并创建邻接矩阵"""
    res = requests.get(url).content.decode()
    node_strs = re.findall(r'node \[.*?\]', res, re.S)
    node_count = len(node_strs)
    edge_strs = re.findall(r'edge \[.*?\]', res, re.S)
    nodes = [{value[0]: value[1] for value in re.findall(r'\s+([^\s]{2,}?)\s+([^\[\]]+?)\n', n)} for n in node_strs]
    edges = [{value[0]: value[1] for value in re.findall(r'\s+([^\s]{2,}?)\s+([^\[\]]+?)\n', e)} for e in edge_strs]
    matrix = [0 if src == dst else 1 if (str(src), str(dst)) in (
        (edge['source'], edge['target']) for edge in edges) else -1 for src in range(node_count) for dst in range(node_count)]
    return matrix


def get_reachablematrix(adjacencyMatrix):
    """
    使用warshell算法通过邻接矩阵计算可达矩阵
    遍历所有节点，考虑使用这个节点转发能否可达
    """
    reachableMatrix = adjacencyMatrix[::]    # 复制矩阵，防止因为对象传递导致修改了邻接矩阵
    nodeNum = int(np.sqrt(len(reachableMatrix)))
    for k in range(nodeNum):  # 遍历转发节点
        for i in range(nodeNum):    # 遍历src
            for j in range(nodeNum):    # 遍历dst
                if reachableMatrix[i*nodeNum+j] == -1:  # 只有不可达的才判断，减少运算量
                    # 如果转发可达，则可达
                    if reachableMatrix[i*nodeNum+k] == 1 and reachableMatrix[k*nodeNum+j] == 1:
                        reachableMatrix[i*nodeNum+j] = 1
    return reachableMatrix


def create_tms_gravity(adjacencyMatrix: list, max_tms: int = -1, total_rate: int = -1, need_print=False):
    """
    使用类引力模型，根据给定的邻接矩阵创建一组业务需求
    @param matrix: 给定的邻接矩阵
    @param max_tms: 最多创建多少个业务需求。如果未指定，max_tms的默认值是边数目的3倍
    @param total_rate: 每个业务中总的速率，即网络负载。如果未指定，总速率是边数
    @return: 返回创建完成的所有TM，包括arg(str，用于传参)和state(list，用作训练state)两组
    """
    edgenum = adjacencyMatrix.count(1)
    nodenum = int(np.sqrt(len(adjacencyMatrix)))
    reachableMatrix = np.array(get_reachablematrix(adjacencyMatrix))
    reachableMatrix = np.reshape(reachableMatrix, (nodenum, nodenum))
    if max_tms == -1:
        # 如果未指定最大创建多少个tm，则通过邻接矩阵计算，最大tm数是边数的3倍
        max_tms = int(edgenum * 3)
    if total_rate == -1:
        # 如果没指定总速率，则通过邻接矩阵计算，总速率是边数的一半
        total_rate = edgenum
    tm_str_list, tm_state_list = [], []
    for _ in range(max_tms):
        a = np.random.exponential(size=nodenum)
        b = np.random.exponential(size=nodenum)
        tm_state = np.outer(a, b)
        tm_state[reachableMatrix!=1] = 0   # 所有不连通的位置流量都置为0
        np.fill_diagonal(tm_state, 0)     # 对角线也置为0
        tm_state[tm_state!=0] = np.asarray(total_rate * tm_state[tm_state!=0]/np.sum(tm_state[tm_state!=0])).clip(min=0.001, max=5)
        tm_state = tm_state.reshape(-1)
        tm_state_list.append(tm_state)
        tm = []
        for src in range(nodenum):
            for dst in range(nodenum):
                rate = tm_state[src * nodenum + dst]
                if rate > 0:
                    tm.append({'src': src, 'dst': dst, 'rate': rate})
        tm_str = '[' + ','.join(['{' + ','.join(['/{}/:{}'.format(k, t[k]) for k in t.keys()]) + '}' for t in tm]) + ']'
        tm_str_list.append(tm_str)
    return tm_str_list, tm_state_list

    
def create_tms(adjacencyMatrix: list, max_tms: int = -1, total_rate: int = -1, need_print=False):
    """
    根据给定的邻接矩阵创建一组业务需求
    @param matrix: 给定的邻接矩阵
    @param max_tms: 最多创建多少个业务需求。如果未指定，max_tms的默认值是边数目的3倍
    @param total_rate: 每个业务中总的速率，即网络负载。如果未指定，总速率是边数
    @return: 返回创建完成的所有TM，包括arg(str，用于传参)和state(list，用作训练state)两组
    """
    max_rate = 4  # * 约定最大rate是4，客观原因是一条信道的容量是5Mbps
    edgenum = adjacencyMatrix.count(1)
    nodenum = int(np.sqrt(len(adjacencyMatrix)))
    reachableMatrix = get_reachablematrix(adjacencyMatrix)
    if max_tms == -1:
        # 如果未指定最大创建多少个tm，则通过邻接矩阵计算，最大tm数是边数的3倍
        max_tms = int(edgenum * 3)
    if total_rate == -1:
        # 如果没指定总速率，则通过邻接矩阵计算，总速率是边数的一半
        # * 注意这个总速率在信道速率为5Mbps的时候才有合理性
        total_rate = edgenum
    tm_arg_list = []
    tm_state_list = []
    while True:
        if need_print:
            print('creating tms', '.' * (len(tm_arg_list) % 6), sep='', end='\r')
        origin_dst_set = set()   # 每个业务TM中从origin到dst的组合出现次数应该是唯一的
        tm = []  # tm中存的是origin-dst-rate构成的dict，可以理解为稀疏矩阵有效的部分
        tm_state = [0 for _ in range(nodenum * nodenum)]  # tm_state中存的是完整的业务矩阵
        res_rate = total_rate         # 直到没有rate可用为止
        attempt = 0
        while res_rate > 0:
            # attempt += 1
            # if attempt >= max_attempt:
            #     break
            src = random.randint(0, nodenum - 1)
            dst = random.randint(0, nodenum - 1)
            if reachableMatrix[src * nodenum + dst] != 1:   # 判断是不是有效路径（可达）
                continue
            if (src, dst) in origin_dst_set:  # 判断是不是重复
                continue
            # 随机一个rate
            rate = random.uniform(1, min(res_rate, max_rate))
            origin_dst_set.add((src, dst))
            tm.append({'src': src, 'dst': dst, 'rate': rate})
            tm_state[src * nodenum + dst] = rate
            res_rate -= rate

        tm_str = '[' + ','.join(['{' + ','.join(['/{}/:{}'.format(k, t[k]) for k in t.keys()]) + '}' for t in tm]) + ']'
        tm_arg_list.append(tm_str)
        tm_state_list.append(tm_state)
        if len(tm_arg_list) == max_tms:
            break
    print('create tms finished')
    return tm_arg_list, tm_state_list


def save_tms(tms, file_path='tms.save'):
    """保存创建好的TMs"""
    ofstr = ''
    for index, tm in enumerate(tms):
        ofstr += f'{index}-{tm}\n'
    with open(file_path, 'w') as f:
        f.write(ofstr)
        f.close()


if __name__ == '__main__':
    # get_reachablematrix([0, 1, 1, -1, -1, 0, -1, 1, -1, -1, 0, 1, -1, -1, -1, 0])
    pprint(create_tms_gravity([0, 1, 1, -1, -1, 0, -1, 1, -1, -1, 0, 1, -1, -1, -1, 0], 4, 4))
