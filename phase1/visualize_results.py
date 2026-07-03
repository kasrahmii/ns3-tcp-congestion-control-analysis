#!/usr/bin/env python3
"""
TCP Simulation Analysis and Visualization
=========================================
این اسکریپت نتایج شبیه‌سازی TCP را تحلیل کرده و نمودارهای مختلف تولید می‌کند.
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from pathlib import Path
import glob

# تنظیمات فونت برای فارسی
plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['axes.unicode_minus'] = False

# تنظیمات استایل
sns.set_style("whitegrid")
sns.set_palette("husl")

class TCPAnalyzer:
    """کلاس تحلیل و نمودارسازی نتایج شبیه‌سازی TCP"""
    
    def __init__(self, results_dir="results"):
        """
        مقداردهی اولیه
        
        Args:
            results_dir: مسیر دایرکتوری حاوی نتایج
        """
        self.results_dir = Path(results_dir)
        self.summary_file = self.results_dir / "summary.csv"
        self.output_dir = self.results_dir / "plots"
        self.output_dir.mkdir(exist_ok=True)
        
        # خواندن داده‌های خلاصه
        self.df = None
        if self.summary_file.exists():
            self.df = pd.read_csv(self.summary_file)
            print(f"✓ Loaded {len(self.df)} simulation results")
        else:
            print(f"✗ Summary file not found: {self.summary_file}")
    
    def plot_throughput_comparison(self):
        """نمودار مقایسه throughput بین الگوریتم‌ها"""
        if self.df is None:
            return
        
        fig, axes = plt.subplots(1, 3, figsize=(18, 5))
        fig.suptitle('Throughput Comparison: TCP Algorithms', fontsize=16, fontweight='bold')
        
        flow_counts = sorted(self.df['Flows'].unique())
        
        for idx, flows in enumerate(flow_counts):
            ax = axes[idx]
            data = self.df[self.df['Flows'] == flows]
            
            # گروه‌بندی بر اساس TCP و PacketRate
            pivot_data = data.pivot(index='PacketRate', columns='TCP', values='AvgThroughput(kbps)')
            
            pivot_data.plot(kind='bar', ax=ax, width=0.8)
            ax.set_title(f'Flows = {flows}', fontsize=12, fontweight='bold')
            ax.set_xlabel('Packet Rate (pkt/s)', fontsize=10)
            ax.set_ylabel('Avg Throughput (kbps)', fontsize=10)
            ax.legend(title='TCP Algorithm', fontsize=9)
            ax.grid(True, alpha=0.3)
            ax.set_xticklabels(ax.get_xticklabels(), rotation=0)
        
        plt.tight_layout()
        output_file = self.output_dir / "throughput_comparison.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def plot_delay_comparison(self):
        """نمودار مقایسه تأخیر بین الگوریتم‌ها"""
        if self.df is None:
            return
        
        fig, axes = plt.subplots(1, 3, figsize=(18, 5))
        fig.suptitle('Delay Comparison: TCP Algorithms', fontsize=16, fontweight='bold')
        
        flow_counts = sorted(self.df['Flows'].unique())
        
        for idx, flows in enumerate(flow_counts):
            ax = axes[idx]
            data = self.df[self.df['Flows'] == flows]
            
            pivot_data = data.pivot(index='PacketRate', columns='TCP', values='AvgDelay(ms)')
            
            pivot_data.plot(kind='bar', ax=ax, width=0.8)
            ax.set_title(f'Flows = {flows}', fontsize=12, fontweight='bold')
            ax.set_xlabel('Packet Rate (pkt/s)', fontsize=10)
            ax.set_ylabel('Avg Delay (ms)', fontsize=10)
            ax.legend(title='TCP Algorithm', fontsize=9)
            ax.grid(True, alpha=0.3)
            ax.set_xticklabels(ax.get_xticklabels(), rotation=0)
        
        plt.tight_layout()
        output_file = self.output_dir / "delay_comparison.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def plot_loss_rate_comparison(self):
        """نمودار مقایسه نرخ از دست رفتن بسته"""
        if self.df is None:
            return
        
        fig, axes = plt.subplots(1, 3, figsize=(18, 5))
        fig.suptitle('Packet Loss Rate Comparison', fontsize=16, fontweight='bold')
        
        flow_counts = sorted(self.df['Flows'].unique())
        
        for idx, flows in enumerate(flow_counts):
            ax = axes[idx]
            data = self.df[self.df['Flows'] == flows]
            
            pivot_data = data.pivot(index='PacketRate', columns='TCP', values='LossRate(%)')
            
            pivot_data.plot(kind='bar', ax=ax, width=0.8)
            ax.set_title(f'Flows = {flows}', fontsize=12, fontweight='bold')
            ax.set_xlabel('Packet Rate (pkt/s)', fontsize=10)
            ax.set_ylabel('Loss Rate (%)', fontsize=10)
            ax.legend(title='TCP Algorithm', fontsize=9)
            ax.grid(True, alpha=0.3)
            ax.set_xticklabels(ax.get_xticklabels(), rotation=0)
        
        plt.tight_layout()
        output_file = self.output_dir / "loss_rate_comparison.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def plot_fairness_comparison(self):
        """نمودار مقایسه شاخص عدالت Jain"""
        if self.df is None:
            return
        
        fig, axes = plt.subplots(1, 3, figsize=(18, 5))
        fig.suptitle("Jain's Fairness Index Comparison", fontsize=16, fontweight='bold')
        
        flow_counts = sorted(self.df['Flows'].unique())
        
        for idx, flows in enumerate(flow_counts):
            ax = axes[idx]
            data = self.df[self.df['Flows'] == flows]
            
            pivot_data = data.pivot(index='PacketRate', columns='TCP', values='JainsFairness')
            
            pivot_data.plot(kind='bar', ax=ax, width=0.8)
            ax.set_title(f'Flows = {flows}', fontsize=12, fontweight='bold')
            ax.set_xlabel('Packet Rate (pkt/s)', fontsize=10)
            ax.set_ylabel("Jain's Fairness Index", fontsize=10)
            ax.set_ylim([0, 1.1])
            ax.legend(title='TCP Algorithm', fontsize=9)
            ax.grid(True, alpha=0.3)
            ax.set_xticklabels(ax.get_xticklabels(), rotation=0)
            
            # خط مرجع برای عدالت کامل
            ax.axhline(y=1.0, color='r', linestyle='--', alpha=0.5, label='Perfect Fairness')
        
        plt.tight_layout()
        output_file = self.output_dir / "fairness_comparison.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def plot_goodput_comparison(self):
        """نمودار مقایسه Goodput کل"""
        if self.df is None:
            return
        
        fig, axes = plt.subplots(1, 3, figsize=(18, 5))
        fig.suptitle('Total Goodput Comparison', fontsize=16, fontweight='bold')
        
        flow_counts = sorted(self.df['Flows'].unique())
        
        for idx, flows in enumerate(flow_counts):
            ax = axes[idx]
            data = self.df[self.df['Flows'] == flows]
            
            pivot_data = data.pivot(index='PacketRate', columns='TCP', values='TotalGoodput(kbps)')
            
            pivot_data.plot(kind='bar', ax=ax, width=0.8)
            ax.set_title(f'Flows = {flows}', fontsize=12, fontweight='bold')
            ax.set_xlabel('Packet Rate (pkt/s)', fontsize=10)
            ax.set_ylabel('Total Goodput (kbps)', fontsize=10)
            ax.legend(title='TCP Algorithm', fontsize=9)
            ax.grid(True, alpha=0.3)
            ax.set_xticklabels(ax.get_xticklabels(), rotation=0)
        
        plt.tight_layout()
        output_file = self.output_dir / "goodput_comparison.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def plot_heatmaps(self):
        """نمودارهای heatmap برای مقایسه کلی"""
        if self.df is None:
            return
        
        metrics = ['AvgThroughput(kbps)', 'AvgDelay(ms)', 'LossRate(%)', 'JainsFairness']
        metric_names = ['Avg Throughput', 'Avg Delay', 'Loss Rate', 'Jain\'s Fairness']
        
        tcp_algorithms = sorted(self.df['TCP'].unique())
        
        for tcp in tcp_algorithms:
            fig, axes = plt.subplots(2, 2, figsize=(14, 12))
            fig.suptitle(f'Performance Heatmaps: {tcp}', fontsize=16, fontweight='bold')
            
            data = self.df[self.df['TCP'] == tcp]
            
            for idx, (metric, metric_name) in enumerate(zip(metrics, metric_names)):
                ax = axes[idx // 2, idx % 2]
                
                # ایجاد pivot table
                pivot = data.pivot(index='Flows', columns='PacketRate', values=metric)
                
                # رسم heatmap
                sns.heatmap(pivot, annot=True, fmt='.2f', cmap='YlOrRd', ax=ax, 
                           cbar_kws={'label': metric_name})
                ax.set_title(metric_name, fontsize=12, fontweight='bold')
                ax.set_xlabel('Packet Rate (pkt/s)', fontsize=10)
                ax.set_ylabel('Number of Flows', fontsize=10)
            
            plt.tight_layout()
            output_file = self.output_dir / f"heatmap_{tcp}.png"
            plt.savefig(output_file, dpi=300, bbox_inches='tight')
            print(f"✓ Saved: {output_file}")
            plt.close()
    
    def plot_cwnd_traces(self, tcp_type='NewReno', flows=20, rate=300, max_flows=5):
        """
        نمودار تغییرات CWND برای جریان‌های مختلف
        
        Args:
            tcp_type: نوع TCP (NewReno, Cubic, Vegas)
            flows: تعداد جریان‌ها
            rate: نرخ بسته
            max_flows: حداکثر تعداد جریان‌هایی که نمایش داده شوند
        """
        cwnd_dir = self.results_dir / "cwnd"
        if not cwnd_dir.exists():
            print(f"✗ CWND directory not found: {cwnd_dir}")
            return
        
        fig, ax = plt.subplots(figsize=(14, 7))
        
        # پیدا کردن فایل‌های cwnd
        pattern = f"cwnd-flow-*.dat"
        cwnd_files = sorted(glob.glob(str(cwnd_dir / pattern)))
        
        if not cwnd_files:
            print(f"✗ No CWND files found in {cwnd_dir}")
            return
        
        # نمایش تعداد محدودی جریان
        for idx, file in enumerate(cwnd_files[:max_flows]):
            try:
                data = pd.read_csv(file, sep='\t', comment='#', 
                                 names=['Time', 'CWND'])
                ax.plot(data['Time'], data['CWND'], 
                       label=f'Flow {idx}', linewidth=1.5, alpha=0.7)
            except Exception as e:
                print(f"✗ Error reading {file}: {e}")
        
        ax.set_title(f'Congestion Window Evolution: {tcp_type} '
                    f'(Flows={flows}, Rate={rate} pkt/s)', 
                    fontsize=14, fontweight='bold')
        ax.set_xlabel('Time (seconds)', fontsize=12)
        ax.set_ylabel('CWND (segments)', fontsize=12)
        ax.legend(fontsize=10, loc='best')
        ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        output_file = self.output_dir / f"cwnd_trace_{tcp_type}_f{flows}_r{rate}.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def plot_multi_metric_comparison(self):
        """نمودار مقایسه چندمعیاره برای یک سناریو خاص"""
        if self.df is None:
            return
        
        # انتخاب یک سناریو میانه (flows=20, rate=300)
        data = self.df[(self.df['Flows'] == 20) & (self.df['PacketRate'] == 300)]
        
        if len(data) == 0:
            print("✗ No data for scenario: flows=20, rate=300")
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('Multi-Metric Comparison: Flows=20, Rate=300 pkt/s', 
                    fontsize=16, fontweight='bold')
        
        tcp_algos = data['TCP'].values
        
        # Throughput
        ax1 = axes[0, 0]
        ax1.bar(tcp_algos, data['AvgThroughput(kbps)'].values, color=['#2ecc71', '#3498db', '#e74c3c'])
        ax1.set_title('Average Throughput', fontweight='bold')
        ax1.set_ylabel('Throughput (kbps)')
        ax1.grid(True, alpha=0.3)
        
        # Delay
        ax2 = axes[0, 1]
        ax2.bar(tcp_algos, data['AvgDelay(ms)'].values, color=['#2ecc71', '#3498db', '#e74c3c'])
        ax2.set_title('Average Delay', fontweight='bold')
        ax2.set_ylabel('Delay (ms)')
        ax2.grid(True, alpha=0.3)
        
        # Loss Rate
        ax3 = axes[1, 0]
        ax3.bar(tcp_algos, data['LossRate(%)'].values, color=['#2ecc71', '#3498db', '#e74c3c'])
        ax3.set_title('Packet Loss Rate', fontweight='bold')
        ax3.set_ylabel('Loss Rate (%)')
        ax3.grid(True, alpha=0.3)
        
        # Fairness
        ax4 = axes[1, 1]
        ax4.bar(tcp_algos, data['JainsFairness'].values, color=['#2ecc71', '#3498db', '#e74c3c'])
        ax4.set_title("Jain's Fairness Index", fontweight='bold')
        ax4.set_ylabel('Fairness Index')
        ax4.set_ylim([0, 1.1])
        ax4.axhline(y=1.0, color='r', linestyle='--', alpha=0.5)
        ax4.grid(True, alpha=0.3)
        
        plt.tight_layout()
        output_file = self.output_dir / "multi_metric_comparison.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def plot_scalability_analysis(self):
        """تحلیل مقیاس‌پذیری: تأثیر تعداد جریان‌ها"""
        if self.df is None:
            return
        
        fig, axes = plt.subplots(2, 2, figsize=(14, 10))
        fig.suptitle('Scalability Analysis: Impact of Flow Count', 
                    fontsize=16, fontweight='bold')
        
        # برای هر نرخ بسته
        rate = 300  # نرخ میانه
        data = self.df[self.df['PacketRate'] == rate]
        
        # Throughput vs Flows
        ax1 = axes[0, 0]
        for tcp in data['TCP'].unique():
            tcp_data = data[data['TCP'] == tcp]
            ax1.plot(tcp_data['Flows'], tcp_data['AvgThroughput(kbps)'], 
                    marker='o', label=tcp, linewidth=2)
        ax1.set_title('Throughput vs Number of Flows', fontweight='bold')
        ax1.set_xlabel('Number of Flows')
        ax1.set_ylabel('Avg Throughput (kbps)')
        ax1.legend()
        ax1.grid(True, alpha=0.3)
        
        # Delay vs Flows
        ax2 = axes[0, 1]
        for tcp in data['TCP'].unique():
            tcp_data = data[data['TCP'] == tcp]
            ax2.plot(tcp_data['Flows'], tcp_data['AvgDelay(ms)'], 
                    marker='o', label=tcp, linewidth=2)
        ax2.set_title('Delay vs Number of Flows', fontweight='bold')
        ax2.set_xlabel('Number of Flows')
        ax2.set_ylabel('Avg Delay (ms)')
        ax2.legend()
        ax2.grid(True, alpha=0.3)
        
        # Loss Rate vs Flows
        ax3 = axes[1, 0]
        for tcp in data['TCP'].unique():
            tcp_data = data[data['TCP'] == tcp]
            ax3.plot(tcp_data['Flows'], tcp_data['LossRate(%)'], 
                    marker='o', label=tcp, linewidth=2)
        ax3.set_title('Loss Rate vs Number of Flows', fontweight='bold')
        ax3.set_xlabel('Number of Flows')
        ax3.set_ylabel('Loss Rate (%)')
        ax3.legend()
        ax3.grid(True, alpha=0.3)
        
        # Fairness vs Flows
        ax4 = axes[1, 1]
        for tcp in data['TCP'].unique():
            tcp_data = data[data['TCP'] == tcp]
            ax4.plot(tcp_data['Flows'], tcp_data['JainsFairness'], 
                    marker='o', label=tcp, linewidth=2)
        ax4.set_title("Fairness vs Number of Flows", fontweight='bold')
        ax4.set_xlabel('Number of Flows')
        ax4.set_ylabel("Jain's Fairness Index")
        ax4.set_ylim([0, 1.1])
        ax4.axhline(y=1.0, color='r', linestyle='--', alpha=0.5)
        ax4.legend()
        ax4.grid(True, alpha=0.3)
        
        plt.tight_layout()
        output_file = self.output_dir / "scalability_analysis.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def generate_summary_table(self):
        """تولید جدول خلاصه نتایج"""
        if self.df is None:
            return
        
        print("\n" + "="*80)
        print("SUMMARY TABLE")
        print("="*80)
        
        # گروه‌بندی بر اساس TCP
        for tcp in sorted(self.df['TCP'].unique()):
            tcp_data = self.df[self.df['TCP'] == tcp]
            print(f"\n{tcp}:")
            print("-" * 80)
            print(tcp_data.to_string(index=False))
        
        print("\n" + "="*80)
        print("BEST PERFORMANCE BY METRIC")
        print("="*80)
        
        metrics = {
            'Highest Throughput': ('AvgThroughput(kbps)', 'max'),
            'Lowest Delay': ('AvgDelay(ms)', 'min'),
            'Lowest Loss Rate': ('LossRate(%)', 'min'),
            'Best Fairness': ('JainsFairness', 'max'),
            'Highest Goodput': ('TotalGoodput(kbps)', 'max')
        }
        
        for name, (metric, agg) in metrics.items():
            if agg == 'max':
                best_row = self.df.loc[self.df[metric].idxmax()]
            else:
                best_row = self.df.loc[self.df[metric].idxmin()]
            
            print(f"\n{name}:")
            print(f"  TCP: {best_row['TCP']}")
            print(f"  Flows: {best_row['Flows']}")
            print(f"  Packet Rate: {best_row['PacketRate']} pkt/s")
            print(f"  Value: {best_row[metric]:.2f}")
    
    def run_all_analysis(self):
        """اجرای تمام تحلیل‌ها و تولید همه نمودارها"""
        print("\n" + "="*80)
        print("TCP SIMULATION ANALYSIS")
        print("="*80 + "\n")
        
        if self.df is None:
            print("✗ No data loaded. Please check the results directory.")
            return
        
        print("Generating plots...")
        print("-" * 80)
        
        self.plot_throughput_comparison()
        self.plot_delay_comparison()
        self.plot_loss_rate_comparison()
        self.plot_fairness_comparison()
        self.plot_goodput_comparison()
        self.plot_heatmaps()
        self.plot_multi_metric_comparison()
        self.plot_scalability_analysis()
        
        # نمودار CWND (اگر داده موجود باشد)
        for tcp in ['NewReno', 'Cubic', 'Vegas']:
            self.plot_cwnd_traces(tcp_type=tcp, flows=20, rate=300, max_flows=5)
        
        self.generate_summary_table()
        
        print("\n" + "="*80)
        print(f"✓ All plots saved to: {self.output_dir}")
        print("="*80 + "\n")


def main():
    """تابع اصلی"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Analyze TCP simulation results')
    parser.add_argument('--results-dir', default='results', 
                       help='Directory containing simulation results')
    parser.add_argument('--plots-only', action='store_true',
                       help='Generate plots only (skip summary table)')
    
    args = parser.parse_args()
    
    analyzer = TCPAnalyzer(args.results_dir)
    
    if args.plots_only:
        print("Generating plots...")
        analyzer.plot_throughput_comparison()
        analyzer.plot_delay_comparison()
        analyzer.plot_loss_rate_comparison()
        analyzer.plot_fairness_comparison()
        analyzer.plot_goodput_comparison()
        analyzer.plot_heatmaps()
        analyzer.plot_multi_metric_comparison()
        analyzer.plot_scalability_analysis()
    else:
        analyzer.run_all_analysis()


if __name__ == "__main__":
    main()

