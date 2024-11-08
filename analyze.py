import re
import matplotlib.pyplot as plt
from datetime import datetime
import numpy as np
from collections import defaultdict

# 设置文件路径和输出路径
input_file = 'data.txt'
figure_output_dir = './writeups/figures/'

# 定义正则表达式来解析ping行
ping_pattern = re.compile(r'\[([\d\.]+)\] 64 bytes from .*: icmp_seq=(\d+) ttl=\d+ time=([\d\.]+) ms')

# 解析ping文件
def parse_ping_log(file_path):
    pings = []
    all_seq = set()
    
    with open(file_path, 'r') as file:
        for line in file:
            match = ping_pattern.search(line)
            if match:
                timestamp, seq, time_ms = match.groups()
                timestamp = float(timestamp)
                seq = int(seq)
                time_ms = float(time_ms)
                pings.append((timestamp, seq, time_ms))
                all_seq.add(seq)
    
    # 检查丢失的ping
    received_seqs = {ping[1] for ping in pings}
    total_requests = max(all_seq)
    lost_seqs = sorted(set(range(1, total_requests + 1)) - received_seqs)
    
    return pings, total_requests, lost_seqs

# 计算总体送达率
def calculate_delivery_rate(total_requests, lost_seqs):
    received_requests = total_requests - len(lost_seqs)
    print(f'Total Requests: {total_requests}')
    print(f'Lost Seqs: {len(lost_seqs)}')
    print(f'Received Requests: {received_requests}')
    return received_requests / total_requests

# 找到最长的成功和丢失的连续段
def find_longest_streaks(pings, total_requests, lost_seqs):
    longest_success_streak = 0
    longest_loss_streak = 0
    
    current_success_streak = 0
    current_loss_streak = 0
    
    # 记录成功和失败状态
    success_status = [1 if i not in lost_seqs else 0 for i in range(1, total_requests + 1)]
    
    for status in success_status:
        if status == 1:
            current_success_streak += 1
            longest_success_streak = max(longest_success_streak, current_success_streak)
            current_loss_streak = 0
        else:
            current_loss_streak += 1
            longest_loss_streak = max(longest_loss_streak, current_loss_streak)
            current_success_streak = 0
    
    return longest_success_streak, longest_loss_streak

# 条件概率计算
def conditional_probabilities(success_status):
    successes_after_success = 0
    total_successes = 0
    successes_after_loss = 0
    total_losses = 0
    
    for i in range(len(success_status) - 1):
        if success_status[i] == 1:
            total_successes += 1
            if success_status[i + 1] == 1:
                successes_after_success += 1
        else:
            total_losses += 1
            if success_status[i + 1] == 1:
                successes_after_loss += 1
    
    prob_success_given_success = successes_after_success / total_successes if total_successes > 0 else 0
    prob_success_given_loss = successes_after_loss / total_losses if total_losses > 0 else 0
    return prob_success_given_success, prob_success_given_loss

# RTT统计
def calculate_rtt_stats(pings):
    rtts = [ping[2] for ping in pings]
    min_rtt = min(rtts)
    max_rtt = max(rtts)
    return min_rtt, max_rtt, rtts

# 绘制RTT随时间变化图
def plot_rtt_vs_time(pings, figure_output_dir):
    times = [datetime.fromtimestamp(ping[0]) for ping in pings]
    rtts = [ping[2] for ping in pings]
    
    plt.figure()
    plt.plot(times, rtts, '-o', markersize=2)
    plt.xlabel("Time")
    plt.ylabel("RTT (ms)")
    plt.title("RTT over Time")
    plt.savefig(f"{figure_output_dir}/rtt_vs_time.png")

# 绘制RTT分布直方图或累积分布
def plot_rtt_distribution(rtts, figure_output_dir):
    plt.figure()
    plt.hist(rtts, bins=50, cumulative=True, density=True)
    plt.xlabel("RTT (ms)")
    plt.ylabel("Cumulative Probability")
    plt.title("Cumulative Distribution of RTT")
    plt.savefig(f"{figure_output_dir}/rtt_distribution.png")

# 绘制RTT相关性图
def plot_rtt_correlation(rtts, figure_output_dir):
    rtts_n = rtts[:-1]
    rtts_n1 = rtts[1:]
    
    plt.figure()
    plt.scatter(rtts_n, rtts_n1, alpha=0.5)
    plt.xlabel("RTT of ping #N (ms)")
    plt.ylabel("RTT of ping #N+1 (ms)")
    plt.title("Correlation between RTT of ping #N and ping #N+1")
    plt.savefig(f"{figure_output_dir}/rtt_correlation.png")

# 主程序
def main():
    # 解析日志文件
    pings, total_requests, lost_seqs = parse_ping_log(input_file)
    
    # 问题1：计算总体送达率
    delivery_rate = calculate_delivery_rate(total_requests, lost_seqs)
    print(f"Overall Delivery Rate: {delivery_rate:.2%}")
    
    # 问题2和3：最长连续成功和失败的段
    longest_success_streak, longest_loss_streak = find_longest_streaks(pings, total_requests, lost_seqs)
    print(f"Longest Consecutive Successful Pings: {longest_success_streak}")
    print(f"Longest Burst of Losses: {longest_loss_streak}")
    
    # 问题4：条件概率
    success_status = [1 if i not in lost_seqs else 0 for i in range(1, total_requests + 1)]
    prob_success_given_success, prob_success_given_loss = conditional_probabilities(success_status)
    print(f"P(Success | Success): {prob_success_given_success:.2%}")
    print(f"P(Success | Loss): {prob_success_given_loss:.2%}")
    
    # 问题5和6：最小和最大RTT
    min_rtt, max_rtt, rtts = calculate_rtt_stats(pings)
    print(f"Minimum RTT: {min_rtt} ms")
    print(f"Maximum RTT: {max_rtt} ms")
    
    # 问题7：RTT随时间变化图
    plot_rtt_vs_time(pings, figure_output_dir)
    
    # 问题8：RTT分布图
    plot_rtt_distribution(rtts, figure_output_dir)
    
    # 问题9：RTT相关性图
    plot_rtt_correlation(rtts, figure_output_dir)
    
    # 问题10：总结
    print("Conclusions: Based on the data, ...")

if __name__ == '__main__':
    main()
