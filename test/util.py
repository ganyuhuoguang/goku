#!/usr/bin/python
# -*- coding:utf-8 -*-
import argparse
import requests
import time
import threading
import sys
import os
import random

URL = "192.168.1.100:20000"
record = []


class Performance:
    def __init__(self, start_time, end_time, response_time, is_success):
        self.response_time = response_time
        self.is_success = is_success
        self.start_time = start_time
        self.end_time = end_time


def verifyOnce(url, video_path, model_group_ids):
    if not(url):
        url = URL
    try:
        #print 'video_path: %s. model_group_ids: %s' % (video_path, model_group_ids)
        post_data = {'video_path': video_path,
                     'model_group_names': model_group_ids}
        start_time = time.time()
        r = requests.post('http://' + url +
                          '/create_task', json=post_data)
        end_time = time.time()
        response_time = r.elapsed.total_seconds()
        #print r.status_code, r.text 
        is_success = False
        if r.status_code == 200:
            is_success = True
        else :
            print (r)
        p = Performance(start_time, end_time, response_time, is_success)
        record.append(p)
    except (Exception, e):
        print ('Error', e)

def verify(url, video_path, model_group_ids, end_time):
    if not(url):
        url = URL
    now = int(round(time.time() * 1000))
    while now <= end_time:
        verifyOnce(url, video_path, model_group_ids)
        now = int(round(time.time() * 1000))


def printRecord(records, parallel):
    print ('-----------------------Performance---------------------------')
    sum_response_time = 0
    sum_success = 0
    record_length = len(records)
    for item in records:
        sum_response_time += item.response_time
        if item.is_success:
            sum_success += 1
    time = records[len(records) - 1].end_time - records[0].start_time
    if parallel:
        print ('CONCURRENT REQUESTS: %d' % (parallel))
    print ('TOTAL REQUESTS: %d' % (record_length))
    print ('TOTAL TIME(s): %s' % (time))
    print ('SUCEED RATE: %.2f%%' % (float(sum_success) / record_length * 100))
    print ('QPS: %.2f' % (sum_success / time))
    print ('Latency: %.2f' % (sum_response_time / record_length))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='actions to test create task api')
    parser.add_argument(
        '--second', type=int, help='operation period seconds')
    parser.add_argument('--parallel', type=int,
                        help='the number of maximum concurrent access')
    parser.add_argument('--name', type=str,
                        help='the model group name(split by ,)')
    parser.add_argument('--path', type=str,
                        help='the video_path')
    parser.add_argument('--web_addr', type=str, help='web ip address \
    with port')
    args = parser.parse_args()
    print (args)
    model_group_names = args.name.split(',')
    video_path = args.path
    if args.second is None:
        verifyOnce(args.web_addr, video_path, model_group_names)
    else:
        current = int(round(time.time() * 1000))
        future = int(round((time.time() + args.second) * 1000))
        threads = []
        while current <= future:
            for i in range(0, args.parallel):
                t = threading.Thread(target=verify, args=(
                    args.web_addr, video_path, model_group_names, future))
                t.daemon = True
                threads.append(t)
            for t in threads:
                t.start()
            for t in threads:
                t.join()
            threads = []
            current = int(round(time.time() * 1000))
        printRecord(record, args.parallel)
