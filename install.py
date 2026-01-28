#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2024 Huawei Device Co., Ltd.
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
from pathlib import Path


def move_files(source_dir, target_dir):
    source_path = Path(source_dir)
    target_path = Path(target_dir)
    
    target_path.mkdir(parents=True, exist_ok=True)
    
    for item in source_path.rglob("*"):
        if item.is_file():
            rel_path = item.relative_to(source_path)
            dest_path = target_path / rel_path
            dest_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(item, dest_path)
            print(f"Copied: {rel_path}")

    print(f"All files moved from {source_dir} to {target_dir}")


def apply_patch(source_dir):
    patch_list = [
        'modifying_driver_search_path.patch',
        'add_thread_poll.patch',
        'hilog_debug.patch',
        'modify_load_function.patch',
    ]

    for patch in patch_list:
        patch_file = os.path.join(source_dir, 'patches', patch)
        patch_file = os.path.abspath(patch_file)
        source_dir_abs = os.path.abspath(source_dir)
        
        patch_cmd = ['patch', '-p1', "--fuzz=0", "--no-backup-if-mismatch", '-i', patch_file, '-d', source_dir_abs]
        try:
            result = subprocess.run(patch_cmd, check=True, text=True, capture_output=True)
            if result.returncode != 0:
                raise subprocess.CalledProcessError(result.returncode, result.args)
            print(f"Successfully applied: {patch}")
        except subprocess.CalledProcessError as e:
            print(f"Error applying patch {patch}: {e}")
            if e.stderr:
                print(f"Error details: {e.stderr}")
        except FileNotFoundError:
            print(f"Patch file not found: {patch_file}")
        except Exception as e:
            print(f"Unexpected error applying patch {patch}: {e}")


def main():
    parser = argparse.ArgumentParser(description='Move files and apply patches')
    parser.add_argument('--source-dir', help='Source directory containing files to move', required=True)
    parser.add_argument('--target-dir', help='Target directory to move files to', required=True)
    
    args = parser.parse_args()
    
    if not os.path.exists(args.source_dir):
        print(f"Error: Source directory does not exist: {args.source_dir}")
        return 1
    
    move_files(args.source_dir, args.target_dir)
    
    apply_patch(args.target_dir)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())