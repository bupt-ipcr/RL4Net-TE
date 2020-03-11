#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-12-07 19:53
@edit time: 2019-12-07 19:59
@desc: DQN 使用的网络结构，其特点是输出各个离散动作值的value，
输出前会经过一个softmax函数
"""


import torch
import torch.nn as nn
import torch.nn.functional as F
CUDA = torch.cuda.is_available()


class SimpleDQNNet(nn.Module):
    """一个只有一层隐藏层的DQN神经网络"""

    def __init__(self, n_states, n_actions, n_neurons=32):
        """
        定义隐藏层和输出层参数
        @param n_obs: number of observations
        @param n_actions: number of actions
        @param n_neurons: number of neurons for the hidden layer
        """
        super(SimpleDQNNet, self).__init__()
        self.fc1 = nn.Linear(n_states, n_neurons)
        self.fc1.weight.data.normal_(0, 0.1)
        self.out = nn.Linear(n_neurons, n_actions)
        self.out.weight.data.normal_(0, 0.1)

    def forward(self, x):
        """
        定义网络结构: 第一层网络->ReLU激活->输出层->softmax->输出
        """
        if CUDA:
            x = x.cuda()
        x = self.fc1(x)
        x = F.relu(x)
        x = self.out(x)
        action_values = F.softmax(x, dim=1)
        return action_values