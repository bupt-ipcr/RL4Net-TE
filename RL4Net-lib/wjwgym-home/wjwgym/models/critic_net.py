#!/usr/bin/env python
# coding=utf-8
"""
@author: Jiawei Wu
@create time: 2019-12-06 23:23
@edit time: 2019-12-07 20:14
@desc: Critic网络。
其特点是只需要一个输出（q值），且不需要softmax
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
CUDA = torch.cuda.is_available()


class SimpleCriticNet(nn.Module):
    """定义Critic的网络结构"""

    def __init__(self, n_states, n_actions, n_neurons=30):
        """
        定义隐藏层和输出层参数
        @param n_obs: number of observations
        @param n_actions: number of actions
        @param n_neurons: 隐藏层神经元数目
        """
        super(SimpleCriticNet, self).__init__()
        self.fc_state = nn.Linear(n_states, n_neurons)
        self.fc_state.weight.data.normal_(0, 0.1)
        self.fc_action = nn.Linear(n_actions, n_neurons)
        self.fc_action.weight.data.normal_(0, 0.1)
        self.out = nn.Linear(n_neurons, 1)
        self.out.weight.data.normal_(0, 0.1)

    def forward(self, s, a):
        """
        定义网络结构: 
        state -> 全连接   -·-->  中间层 -> 全连接 -> ReLU -> Q值
        action -> 全连接  /相加，偏置
        """
        s, a = (s.cuda(), a.cuda()) if CUDA else (s, a)
        x_s = self.fc_state(s)
        x_a = self.fc_action(a)
        x = F.relu(x_s+x_a)
        actions_value = self.out(x)
        return actions_value
