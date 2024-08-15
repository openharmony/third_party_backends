#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2023 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import tarfile
import argparse
import os
import subprocess
import sys
import shutil

def apply_patch(source_dir):
    patch_list = [
        'usbmanager.patch'
    ]

    for patch in patch_list:
        patch_dir = os.path.join(source_dir, 'patches', patch)
        try :
            patch_cmd = ['patch', '-p1', "--fuzz=0", "--no-backup-if-mismatch", '-i', patch_dir, '-d', source_dir]
            subprocess.run(patch_cmd, check=True)
        except Exception as e:
            print("apply_patch error, revserse patch")
            patch_cmd = ['patch', '-R', '-p1', "--fuzz=0", "--no-backup-if-mismatch", '-i', patch_dir, '-d', source_dir]
            subprocess.run(patch_cmd, check=True)
            continue

def cp_file(source_dir, gen_dir):
    src_list = [
        'usb_manager.cpp',
        'sanei_usb.c'
    ]
    head_list = [
        'usb_manager.h',
        'config.h'
    ]
    for src in src_list:
        source_file = os.path.join(source_dir, 'sanei', src)
        dest_file = os.path.join(gen_dir, 'src', src)
        cp_cmd = ['cp', source_file, dest_file]
        subprocess.run(cp_cmd, check=True)
    for head in head_list:
        source_file = os.path.join(source_dir, 'include', 'sane', head)
        dest_file = os.path.join(gen_dir, 'include', head)
        cp_cmd = ['cp', source_file, dest_file]
        subprocess.run(cp_cmd, check=True)

def main():
    cups_path = argparse.ArgumentParser()
    cups_path.add_argument('--gen-dir', help='generate path of log', required=True)
    cups_path.add_argument('--source-dir', help='generate path of log', required=True)
    args = cups_path.parse_args()
    # gen_dir = "${target_gen_dir}/sane"
    gen_dir = args.gen_dir
    # source_dir = //third_party/backends
    source_dir = args.source_dir
    apply_patch(source_dir)
    cp_file(source_dir, gen_dir)
    return 0

if __name__ == '__main__':
    sys.exit(main())
