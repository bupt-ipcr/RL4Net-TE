import os
import argparse
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
    文件会被放置在ns3gym安装包文件夹下，随ns3gym包一起被安装
    """

    # 产生conf
    waf_path = ns3_path / 'waf'
    conf = {'waf_path': str(waf_path)}

    # 获取conf path
    conf_path = cur_path / 'ns3gym' / 'ns3gym' / 'wafconf.py'
    with conf_path.open('w') as f:
        f.write(f'conf = {conf}')

    print("生成waf路径完成")


def file_copy():
    """
    进行文件复制操作
    """
    # 将opengym文件夹复制到 ns3path/src下

    # 将ns3src/下文件夹复制到 ns3path/src 对应文件夹下
    src_path = ns3_path / 'src'
    opengym_path = cur_path / 'opengym'

    os.system(f"cp -r {opengym_path.resolve()} {src_path.resolve()}")
    print('文件复制完成')


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
