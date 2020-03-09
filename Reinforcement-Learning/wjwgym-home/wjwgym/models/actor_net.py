#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-12-06 23:23
@edit time: 2019-12-12 11:11
@desc: DDPG中Actor使用的网络
特点是有一个bound的特殊参数。因为Actor往往只输出一个动作，但是有上下限.
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
CUDA = torch.cuda.is_available()


class SimpleActorNet(nn.Module):
    """定义Actor的网络结构"""

    def __init__(self, n_states, n_actions, n_neurons=30, a_bound=1):
        """
        定义隐藏层和输出层参数
        @param n_obs: number of observations
        @param n_actions: number of actions
        @param n_neurons: 隐藏层神经元数目
        @param a_bound: action的倍率
        """
        super(SimpleActorNet, self).__init__()
        self.bound = a_bound
        self.fc1 = nn.Linear(n_states, n_neurons)
        self.fc1.weight.data.normal_(0, 0.1)
        self.out = nn.Linear(n_neurons, n_actions)
        self.out.weight.data.normal_(0, 0.1)
        if CUDA:
            self.bound = torch.FloatTensor([self.bound]).cuda()
        else:
            self.bound = torch.FloatTensor([self.bound])

    def forward(self, x):
        """
        定义网络结构: 第一层网络->ReLU激活->输出层->tanh激活->softmax->输出
        """
        x = x.cuda() if CUDA else x
        x = self.fc1(x)
        x = F.relu(x)
        x = self.out(x)
        action_value = F.tanh(x)
        action_value = action_value * self.bound
        return action_value

