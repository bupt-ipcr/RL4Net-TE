#!/usr/bin/env python
# -*- coding: UTF-8 -*-
"""
@author: Jiawei Wu
@create time: 2020-03-17 20:52
@edit time: 2020-04-06 11:35
@FilePath: /ns3-env/env_setup.py
@desc: 
"""

import argparse
import json
import os
from pathlib import Path


parser = argparse.ArgumentParser(description='安装参数')
parser.add_argument('--wafdir', default='/ns-3-dev', type=str, help='安装目录，默认值为/ns-3-dev')
parser.add_argument('--nocopy', default=False, action='store_true', help='是否跳过文件复制')
parser.add_argument('--norebuild', default=False, action='store_true', help='是否跳过waf build')
parser.add_argument('--noreconf', default=False, action='store_true', help='是否跳过waf configure')
parser.add_argument('--noconf', default=False, action='store_true', help='是否跳过conf生成')
args = parser.parse_args()

cur_path = Path().resolve()
ns3_path = (cur_path / args.wafdir).resolve()


def create_conf():
    """
    产生查找waf路径使用的configure文件
    内容是waf路径
    文件会被放置在pyns3安装包文件夹下，随pyns3包一起被安装
    """

    # 产生conf
    waf_path = ns3_path / 'waf'
    conf = json.dumps({'waf_path': str(waf_path)})

    # 获取conf path
    conf_path = cur_path / 'ns3-python-connector' / 'pyns3' / 'wafconf.py'
    with conf_path.open('w') as f:
        f.write(f'conf = {conf}')

    print("Generate wafpath for pyns3 finished.")


def file_copy():
    """
    进行文件复制操作
    """
    src_path = ns3_path / 'src'
    # 将openenv文件夹复制到 ns3path/src下
    print(f'copy openenv to {src_path.resolve()}')
    # 将ns3src/下文件夹复制到 ns3path/src 对应文件夹下
    interface_path = cur_path / 'env-interface'
    openenv_path = cur_path / 'openenv'
    # 创建临时文件夹
    os.system(f'cp -r {interface_path.resolve()} {openenv_path.resolve()}')
    # 文件拷贝
    os.system(f"cp -r {openenv_path.resolve()} {src_path.resolve()}")
    # 删除临时文件夹
    os.system(f'rm -r {openenv_path.resolve()}')
    print('openenv file copy finished.')


def waf_reconf():
    waf_path = ns3_path / 'waf'
    os.chdir(ns3_path.resolve())
    # 执行指令
    print(f'\n{waf_path} configure\n')
    os.system(f'{waf_path} configure')


def waf_rebuild():
    # 定位waf
    waf_path = ns3_path / 'waf'
    os.chdir(ns3_path.resolve())
    # 执行指令
    print(f'\n\n{waf_path} build\n\n')
    os.system(f'{waf_path} build')


if __name__ == '__main__':
    if not args.nocopy:
        file_copy()
    if not args.noreconf:
        waf_reconf()
    if not args.norebuild:
        waf_rebuild()
    if not args.noconf:
        create_conf()
