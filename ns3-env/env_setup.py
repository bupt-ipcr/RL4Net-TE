#!/usr/bin/env python
# -*- coding: UTF-8 -*-
"""
@author: Jiawei Wu
@create time: 2020-03-17 20:52
@edit time: 2020-04-20 21:12
@FilePath: /ns3-env/env_setup.py
@desc: 
"""

import argparse
import json
import os
from pathlib import Path
import re
import sys


parser = argparse.ArgumentParser(description='安装参数')
parser.add_argument('--wafdir', default='/ns-3-dev', type=str, help='安装目录，默认值为/ns-3-dev')
parser.add_argument('--nocopy', default=False, action='store_true', help='是否跳过文件复制')
parser.add_argument('--norebuild', default=False, action='store_true', help='是否跳过waf build')
parser.add_argument('--noreconf', default=False, action='store_true', help='是否跳过waf configure')
parser.add_argument('--noconf', default=False, action='store_true', help='是否跳过conf生成')
parser.add_argument('--noprotoc', default=False, action='store_true', help='是否跳过protoc文件生成')
args = parser.parse_args()

cur_path = Path().resolve()
ns3_path = (cur_path / args.wafdir).resolve()

def error(msg):
    return f"\033[1;31;49m{msg}\033[0m"

def warning(msg):
    return f"\033[0;33;49m{msg}\033[0m"
    
def modend(msg):
    return f"\033[0;32;49m{msg}\033[0m"

def modstart(msg):
    return f"\033[1;34;49m{msg}\033[0m"

def info(msg):
    return f"\033[1;37;49m{msg}\033[0m"

def create_conf():
    """
    产生查找waf路径使用的configure文件
    内容是waf路径
    文件会被放置在pyns3安装包文件夹下，随pyns3包一起被安装
    """

    # 产生conf
    print(modstart(f"Create configure file for waf_path.\n"))
    waf_path = ns3_path / 'waf'
    conf = json.dumps({'waf_path': str(waf_path)})

    # 获取conf path
    conf_path = cur_path / 'ns3-python-connector' / 'pyns3' / 'wafconf.py'
    with conf_path.open('w') as f:
        f.write(f'conf = {conf}')



def create_protoc_pb():
    """利用protoc产生交互需要的pb文件"""
    print(modstart(F"Create pb file using protoc."))
    pyns3_path = cur_path / 'ns3-python-connector' / 'pyns3'
    protobufFile = pyns3_path / 'messages_pb2.py'

    if protobufFile.exists():
        # 提示文件已存在
        print(warning(f"{protobufFile} already exists\n"))
    else:
        print(warning(f"{protobufFile} not found, try to generate\n"))
        # 尝试创建文件
        if not os.path.isfile(protobufFile):
            # 如果没有protocbuf文件，则尝试产生
            rc = os.system('whereis protoc')
            # 先检查是否有whereis指令
            if rc != 0:
                print(error(f"Command: whereis protoc not found"))
                sys.exit("Missing command 'whereis', please check!")
            protoc = None
            # 再查找protoc指令
            rt = os.popen("whereis protoc").read()
            protoc_paths = re.finditer(r'/[^\s]+', rt)
            for protoc_path in protoc_paths:
                if 'conda' in protoc_path[0]:
                    continue
                else:
                    protoc = protoc_path[0]
                    break
            # 如果没有找到protoc，则需要报错
            if protoc is None:
                print(error(f"No path of protoc was found"))
                sys.exit('Protocol Buffer messages are missing. Please run ./waf configure to generate the file')
            # 如果有，则尝试产生文件
            proto = pyns3_path / 'messages.proto'
            print(info(f"{protoc} --python_out={pyns3_path.resolve()} -I={pyns3_path.resolve()} {proto.resolve()}"))
            os.system(f'{protoc} --python_out={pyns3_path.resolve()} -I={pyns3_path.resolve()} {proto.resolve()}')
        print(modend(f"generate pb file success\n"))


def file_copy():
    """
    进行文件复制操作
    """
    src_path = ns3_path / 'src'
    # 将openenv文件夹复制到 ns3path/src下
    print(modstart(f'Copy openenv to {src_path.resolve()}\n'))
    # 将ns3src/下文件夹复制到 ns3path/src 对应文件夹下
    interface_path = cur_path / 'env-interface'
    openenv_path = cur_path / 'openenv'
    # 创建临时文件夹
    os.system(f'cp -r {interface_path.resolve()} {openenv_path.resolve()}')
    # 文件拷贝
    os.system(f"cp -r {openenv_path.resolve()} {src_path.resolve()}")
    # 删除临时文件夹
    os.system(f'rm -r {openenv_path.resolve()}')
    print(modend('openenv file copy finished.'))


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
    if not args.noprotoc:
        create_protoc_pb()
    print("\033[1;36;49mEnv files install finished.\033[0m\n")
