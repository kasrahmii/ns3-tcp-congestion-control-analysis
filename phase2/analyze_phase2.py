#!/usr/bin/env python3
"""
TCP Phase 2 Simulation Analysis and Visualization
=================================================
این اسکریپت نتایج شبیه‌سازی TCP Phase 2 را برای سه توپولوژی مختلف تحلیل کرده و نمودارهای مختلف تولید می‌کند.
"""

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from pathlib import Path
import glob

# تنظیمات فونت
plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['axes.unicode_minus'] = False

# تنظیمات استایل
sns.set_style("whitegrid")
sns.set_palette("husl")

class TCPPhase2Analyzer:
    """کلاس تحلیل و نمودارسازی نتایج شبیه‌سازی TCP Phase 2"""
    
    def __init__(self, results_dir="results_phase2"):
        """
        مقداردهی اولیه
        
        Args:
            results_dir: مسیر دایرکتوری حاوی نتایج
        """
        self.results_dir = Path(results_dir)
        self.summary_file = self.results_dir / "phase2_summary.csv"
        self.output_dir = self.results_dir / "plots"
        self.output_dir.mkdir(exist_ok=True)
        
        # خواندن داده‌های خلاصه
        self.df = None
        if self.summary_file.exists():
            self.df = pd.read_csv(self.summary_file)
            print(f"✓ Loaded {len(self.df)} simulation results")
            print(f"  Topologies: {self.df['Topology'].unique()}")
            print(f"  TCP Variants: {self.df['TCP'].unique()}")
        else:
            print(f"✗ Summary file not found: {self.summary_file}")
    
    def plot_topology_comparison(self):
        """نمودار مقایسه عملکرد بین توپولوژی‌های مختلف"""
        if self.df is None:
            return
        
        metrics = [
            ('AvgThroughput(kbps)', 'Average Throughput (kbps)'),
            ('AvgDelay(ms)', 'Average Delay (ms)'),
            ('LossRate(%)', 'Packet Loss Rate (%)'),
            ('JainsFairness', "Jain's Fairness Index")
        ]
        
        fig, axes = plt.subplots(2, 2, figsize=(16, 12))
        fig.suptitle('Topology Comparison: All TCP Variants', fontsize=16, fontweight='bold')
        
        for idx, (metric, ylabel) in enumerate(metrics):
            ax = axes[idx // 2, idx % 2]
            
            # گروه‌بندی بر اساس توپولوژی و TCP
            topologies = sorted(self.df['Topology'].unique())
            tcp_types = sorted(self.df['TCP'].unique())
            
            x = np.arange(len(topologies))
            width = 0.25
            
            for i, tcp in enumerate(tcp_types):
                tcp_data = self.df[self.df['TCP'] == tcp].groupby('Topology')[metric].mean()
                values = [tcp_data.get(topo, 0) for topo in topologies]
                ax.bar(x + i * width, values, width, label=tcp, alpha=0.8)
            
            ax.set_xlabel('Topology', fontsize=11, fontweight='bold')
            ax.set_ylabel(ylabel, fontsize=11, fontweight='bold')
            ax.set_title(ylabel, fontsize=12, fontweight='bold')
            ax.set_xticks(x + width)
            ax.set_xticklabels(topologies)
            ax.legend()
            ax.grid(True, alpha=0.3, axis='y')
            
            if metric == 'JainsFairness':
                ax.set_ylim([0, 1.1])
                ax.axhline(y=1.0, color='r', linestyle='--', alpha=0.5, linewidth=1)
        
        plt.tight_layout()
        output_file = self.output_dir / "topology_comparison.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def plot_tcp_comparison_by_topology(self):
        """نمودار مقایسه الگوریتم‌های TCP برای هر توپولوژی"""
        if self.df is None:
            return
        
        topologies = sorted(self.df['Topology'].unique())
        
        for topology in topologies:
            fig, axes = plt.subplots(2, 2, figsize=(14, 10))
            fig.suptitle(f'TCP Algorithm Comparison: {topology} Topology', 
                        fontsize=16, fontweight='bold')
            
            topo_data = self.df[self.df['Topology'] == topology]
            tcp_types = sorted(topo_data['TCP'].unique())
            
            # Throughput
            ax1 = axes[0, 0]
            for tcp in tcp_types:
                tcp_data = topo_data[topo_data['TCP'] == tcp]
                ax1.plot(tcp_data['Nodes'], tcp_data['AvgThroughput(kbps)'], 
                        marker='o', label=tcp, linewidth=2, markersize=8)
            ax1.set_xlabel('Parameter (Nodes/Depth)', fontweight='bold')
            ax1.set_ylabel('Avg Throughput (kbps)', fontweight='bold')
            ax1.set_title('Average Throughput', fontsize=12, fontweight='bold')
            ax1.legend()
            ax1.grid(True, alpha=0.3)
            
            # Delay
            ax2 = axes[0, 1]
            for tcp in tcp_types:
                tcp_data = topo_data[topo_data['TCP'] == tcp]
                ax2.plot(tcp_data['Nodes'], tcp_data['AvgDelay(ms)'], 
                        marker='s', label=tcp, linewidth=2, markersize=8)
            ax2.set_xlabel('Parameter (Nodes/Depth)', fontweight='bold')
            ax2.set_ylabel('Avg Delay (ms)', fontweight='bold')
            ax2.set_title('Average Delay', fontsize=12, fontweight='bold')
            ax2.legend()
            ax2.grid(True, alpha=0.3)
            
            # Loss Rate
            ax3 = axes[1, 0]
            for tcp in tcp_types:
                tcp_data = topo_data[topo_data['TCP'] == tcp]
                ax3.plot(tcp_data['Nodes'], tcp_data['LossRate(%)'], 
                        marker='^', label=tcp, linewidth=2, markersize=8)
            ax3.set_xlabel('Parameter (Nodes/Depth)', fontweight='bold')
            ax3.set_ylabel('Loss Rate (%)', fontweight='bold')
            ax3.set_title('Packet Loss Rate', fontsize=12, fontweight='bold')
            ax3.legend()
            ax3.grid(True, alpha=0.3)
            
            # Fairness
            ax4 = axes[1, 1]
            for tcp in tcp_types:
                tcp_data = topo_data[topo_data['TCP'] == tcp]
                ax4.plot(tcp_data['Nodes'], tcp_data['JainsFairness'], 
                        marker='D', label=tcp, linewidth=2, markersize=8)
            ax4.set_xlabel('Parameter (Nodes/Depth)', fontweight='bold')
            ax4.set_ylabel("Jain's Fairness Index", fontweight='bold')
            ax4.set_title("Jain's Fairness Index", fontsize=12, fontweight='bold')
            ax4.set_ylim([0, 1.1])
            ax4.axhline(y=1.0, color='r', linestyle='--', alpha=0.5)
            ax4.legend()
            ax4.grid(True, alpha=0.3)
            
            plt.tight_layout()
            output_file = self.output_dir / f"tcp_comparison_{topology.lower()}.png"
            plt.savefig(output_file, dpi=300, bbox_inches='tight')
            print(f"✓ Saved: {output_file}")
            plt.close()
    
    def plot_heatmaps_per_topology(self):
        """نمودارهای heatmap برای هر توپولوژی"""
        if self.df is None:
            return
        
        topologies = sorted(self.df['Topology'].unique())
        
        for topology in topologies:
            topo_data = self.df[self.df['Topology'] == topology]
            tcp_types = sorted(topo_data['TCP'].unique())
            
            for tcp in tcp_types:
                fig, axes = plt.subplots(2, 2, figsize=(14, 12))
                fig.suptitle(f'{topology} Topology - {tcp}', 
                           fontsize=16, fontweight='bold')
                
                tcp_data = topo_data[topo_data['TCP'] == tcp]
                
                metrics = [
                    ('AvgThroughput(kbps)', 'Avg Throughput (kbps)', 'YlGn'),
                    ('AvgDelay(ms)', 'Avg Delay (ms)', 'YlOrRd'),
                    ('LossRate(%)', 'Loss Rate (%)', 'Reds'),
                    ('JainsFairness', "Jain's Fairness", 'RdYlGn')
                ]
                
                for idx, (metric, title, cmap) in enumerate(metrics):
                    ax = axes[idx // 2, idx % 2]
                    
                    # ساخت pivot table (اگر داده کافی وجود داشت)
                    if len(tcp_data) > 1:
                        pivot = tcp_data.pivot_table(
                            values=metric, 
                            index='Nodes', 
                            aggfunc='mean'
                        )
                        
                        # اگر فقط یک ستون داریم، heatmap ساده
                        data_matrix = pivot.values.reshape(-1, 1)
                        
                        sns.heatmap(data_matrix, annot=True, fmt='.2f', 
                                  cmap=cmap, ax=ax, 
                                  yticklabels=pivot.index,
                                  xticklabels=['Value'],
                                  cbar_kws={'label': title})
                    else:
                        # اگر فقط یک ردیف داریم
                        value = tcp_data[metric].values[0]
                        ax.text(0.5, 0.5, f'{value:.2f}', 
                               ha='center', va='center', fontsize=20)
                        ax.set_title(title)
                        ax.axis('off')
                    
                    ax.set_title(title, fontsize=12, fontweight='bold')
                
                plt.tight_layout()
                output_file = self.output_dir / f"heatmap_{topology.lower()}_{tcp}.png"
                plt.savefig(output_file, dpi=300, bbox_inches='tight')
                print(f"✓ Saved: {output_file}")
                plt.close()
    
    def plot_goodput_comparison(self):
        """نمودار مقایسه Goodput کل"""
        if self.df is None:
            return
        
        fig, ax = plt.subplots(figsize=(12, 7))
        
        topologies = sorted(self.df['Topology'].unique())
        tcp_types = sorted(self.df['TCP'].unique())
        
        x = np.arange(len(topologies))
        width = 0.25
        
        for i, tcp in enumerate(tcp_types):
            tcp_data = self.df[self.df['TCP'] == tcp].groupby('Topology')['TotalGoodput(kbps)'].mean()
            values = [tcp_data.get(topo, 0) for topo in topologies]
            ax.bar(x + i * width, values, width, label=tcp, alpha=0.8)
        
        ax.set_xlabel('Topology', fontsize=12, fontweight='bold')
        ax.set_ylabel('Total Goodput (kbps)', fontsize=12, fontweight='bold')
        ax.set_title('Total Goodput Comparison Across Topologies', 
                    fontsize=14, fontweight='bold')
        ax.set_xticks(x + width)
        ax.set_xticklabels(topologies)
        ax.legend(title='TCP Algorithm', fontsize=10)
        ax.grid(True, alpha=0.3, axis='y')
        
        plt.tight_layout()
        output_file = self.output_dir / "goodput_comparison.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def plot_cwnd_traces(self, topology='Dumbbell', tcp='TcpNewReno', param='3', max_flows=5):
        """
        نمودار تغییرات CWND برای جریان‌های مختلف
        
        Args:
            topology: نوع توپولوژی
            tcp: نوع TCP
            param: پارامتر (تعداد نودها یا عمق)
            max_flows: حداکثر تعداد جریان‌های نمایش داده شده
        """
        cwnd_dir = self.results_dir / "cwnd"
        if not cwnd_dir.exists():
            print(f"✗ CWND directory not found: {cwnd_dir}")
            return
        
        # پیدا کردن فایل‌های مربوطه
        if topology.lower() == 'tree':
            pattern = f"{topology.lower()}_{tcp}_d{param}_flow*.csv"
        else:
            pattern = f"{topology.lower()}_{tcp}_n{param}_flow*.csv"
        
        cwnd_files = sorted(glob.glob(str(cwnd_dir / pattern)))
        
        if not cwnd_files:
            print(f"✗ No CWND files found for pattern: {pattern}")
            return
        
        fig, ax = plt.subplots(figsize=(14, 7))
        
        for idx, file in enumerate(cwnd_files[:max_flows]):
            try:
                data = pd.read_csv(file)
                ax.plot(data['Time'], data['Cwnd'], 
                       label=f'Flow {idx}', linewidth=1.5, alpha=0.7)
            except Exception as e:
                print(f"✗ Error reading {file}: {e}")
        
        ax.set_title(f'Congestion Window Evolution: {topology} - {tcp} (param={param})', 
                    fontsize=14, fontweight='bold')
        ax.set_xlabel('Time (seconds)', fontsize=12)
        ax.set_ylabel('CWND (segments)', fontsize=12)
        ax.legend(fontsize=10, loc='best')
        ax.grid(True, alpha=0.3)
        
        plt.tight_layout()
        output_file = self.output_dir / f"cwnd_{topology.lower()}_{tcp}_p{param}.png"
        plt.savefig(output_file, dpi=300, bbox_inches='tight')
        print(f"✓ Saved: {output_file}")
        plt.close()
    
    def plot_performance_radar(self):
        """نمودار راداری برای مقایسه چند معیاره"""
        if self.df is None:
            return
        
        from math import pi
        
        # نرمال‌سازی داده‌ها برای نمودار راداری
        def normalize(values):
            vmin, vmax = values.min(), values.max()
            if vmax - vmin == 0:
                return pd.Series([0.5] * len(values), index=values.index)
            return (values - vmin) / (vmax - vmin)
        
        topologies = sorted(self.df['Topology'].unique())
        
        for topology in topologies:
            fig, axes = plt.subplots(1, 3, figsize=(18, 6), subplot_kw=dict(projection='polar'))
            fig.suptitle(f'Performance Radar Chart: {topology} Topology', 
                        fontsize=16, fontweight='bold')
            
            topo_data = self.df[self.df['Topology'] == topology]
            tcp_types = sorted(topo_data['TCP'].unique())
            
            categories = ['Throughput', 'Fairness', 'Goodput', 
                        'Low Delay', 'Low Loss']
            N = len(categories)
            angles = [n / float(N) * 2 * pi for n in range(N)]
            angles += angles[:1]
            
            # نرمال‌سازی معیارها برای کل توپولوژی
            tput_norm = normalize(topo_data['AvgThroughput(kbps)'])
            goodput_norm = normalize(topo_data['TotalGoodput(kbps)'])
            delay_norm = normalize(topo_data['AvgDelay(ms)'])
            loss_norm = normalize(topo_data['LossRate(%)'])
            
            for idx, tcp in enumerate(tcp_types):
                ax = axes[idx]
                
                # فیلتر کردن داده‌های این TCP
                tcp_mask = topo_data['TCP'] == tcp
                tcp_indices = topo_data[tcp_mask].index
                
                # محاسبه میانگین برای هر معیار
                values = [
                    tput_norm.loc[tcp_indices].mean(),
                    topo_data.loc[tcp_indices, 'JainsFairness'].mean(),
                    goodput_norm.loc[tcp_indices].mean(),
                    1 - delay_norm.loc[tcp_indices].mean(),
                    1 - loss_norm.loc[tcp_indices].mean()
                ]
                values += values[:1]
                
                ax.plot(angles, values, 'o-', linewidth=2, label=tcp)
                ax.fill(angles, values, alpha=0.25)
                ax.set_xticks(angles[:-1])
                ax.set_xticklabels(categories, size=10)
                ax.set_ylim(0, 1)
                ax.set_title(tcp, fontsize=12, fontweight='bold', pad=20)
                ax.grid(True)
            
            plt.tight_layout()
            output_file = self.output_dir / f"radar_{topology.lower()}.png"
            plt.savefig(output_file, dpi=300, bbox_inches='tight')
            print(f"✓ Saved: {output_file}")
            plt.close()

    
    def generate_summary_table(self):
        """تولید جدول خلاصه نتایج"""
        if self.df is None:
            return
        
        print("\n" + "="*100)
        print("PHASE 2 SUMMARY TABLE")
        print("="*100)
        
        # گروه‌بندی بر اساس توپولوژی و TCP
        for topology in sorted(self.df['Topology'].unique()):
            print(f"\n{'='*100}")
            print(f"{topology.upper()} TOPOLOGY")
            print(f"{'='*100}")
            
            topo_data = self.df[self.df['Topology'] == topology]
            
            for tcp in sorted(topo_data['TCP'].unique()):
                tcp_data = topo_data[topo_data['TCP'] == tcp]
                print(f"\n{tcp}:")
                print("-" * 100)
                print(tcp_data.to_string(index=False))
        
        print("\n" + "="*100)
        print("BEST PERFORMANCE BY METRIC (OVERALL)")
        print("="*100)
        
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
            print(f"  Topology: {best_row['Topology']}")
            print(f"  TCP: {best_row['TCP']}")
            print(f"  Nodes/Depth: {best_row['Nodes']}")
            print(f"  Flows: {best_row['Flows']}")
            print(f"  Value: {best_row[metric]:.2f}")
        
        # آمار توپولوژی‌ها
        print("\n" + "="*100)
        print("TOPOLOGY STATISTICS")
        print("="*100)
        
        for topology in sorted(self.df['Topology'].unique()):
            topo_data = self.df[self.df['Topology'] == topology]
            print(f"\n{topology}:")
            print(f"  Avg Throughput: {topo_data['AvgThroughput(kbps)'].mean():.2f} kbps")
            print(f"  Avg Delay: {topo_data['AvgDelay(ms)'].mean():.2f} ms")
            print(f"  Avg Loss Rate: {topo_data['LossRate(%)'].mean():.2f}%")
            print(f"  Avg Fairness: {topo_data['JainsFairness'].mean():.3f}")
            print(f"  Avg Goodput: {topo_data['TotalGoodput(kbps)'].mean():.2f} kbps")
    
    def run_all_analysis(self):
        """اجرای تمام تحلیل‌ها"""
        print("\n" + "="*100)
        print("TCP PHASE 2 SIMULATION ANALYSIS")
        print("="*100 + "\n")
        
        if self.df is None:
            print("✗ No data loaded. Please check the results directory.")
            return
        
        print("Generating plots...")
        print("-" * 100)
        
        # نمودارهای اصلی
        self.plot_topology_comparison()
        self.plot_tcp_comparison_by_topology()
        self.plot_goodput_comparison()
        self.plot_heatmaps_per_topology()
        self.plot_performance_radar()
        
        # نمودارهای CWND (نمونه)
        topologies = ['Dumbbell', 'Bus', 'Tree']
        tcp_types = ['TcpNewReno', 'TcpCubic', 'TcpVegas']
        params = {'Dumbbell': '3', 'Bus': '6', 'Tree': '2'}
        
        for topology in topologies:
            for tcp in tcp_types:
                self.plot_cwnd_traces(topology=topology, tcp=tcp, 
                                     param=params[topology], max_flows=5)
        
        # جدول خلاصه
        self.generate_summary_table()
        
        print("\n" + "="*100)
        print(f"✓ All plots saved to: {self.output_dir}")
        print("="*100 + "\n")


def main():
    """تابع اصلی"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Analyze TCP Phase 2 simulation results')
    parser.add_argument('--results-dir', default='results_phase2', 
                       help='Directory containing simulation results')
    parser.add_argument('--plots-only', action='store_true',
                       help='Generate plots only (skip summary table)')
    
    args = parser.parse_args()
    
    analyzer = TCPPhase2Analyzer(args.results_dir)
    
    if args.plots_only:
        print("Generating plots...")
        analyzer.plot_topology_comparison()
        analyzer.plot_tcp_comparison_by_topology()
        analyzer.plot_goodput_comparison()
        analyzer.plot_heatmaps_per_topology()
        analyzer.plot_performance_radar()
    else:
        analyzer.run_all_analysis()


if __name__ == "__main__":
    main()

