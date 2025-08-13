import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib import font_manager


# 列出所有系统字体路径
font_paths = font_manager.findSystemFonts(fontpaths=None, fontext='ttf')
print("系统中的字体文件路径：")
for path in font_paths:
    print(path)


# 中文字体路径（确保路径正确）
font_path = '/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc'  # 举例
font = font_manager.FontProperties(fname=font_path)

# 设置全局默认字体为中文字体
plt.rcParams['font.family'] = font.get_name()
plt.rcParams['axes.unicode_minus'] = False


# --- Step 1: 读取数据 ---
df = pd.read_csv('latency_log.csv',header=0)

# 转换数据类型（确保 trace_id 是整数）
df['TraceID'] = df['TraceID'].astype(int)

# --- Step 2: 重塑表格结构 ---
pivot = df.pivot_table(
    index=['Ticker', 'TraceID'],
    columns='EventTag',
    values='Timestamp_ns',
    aggfunc='first'  # 多次出现时只取第一次
)

print("⚠️ 可用列名：", pivot.columns.tolist())

# --- Step 3: 计算阶段性延迟（单位纳秒） ---
pivot['T2-T1'] = pivot['T2_market_signal_queue_push'] - pivot['T1_MD_received']
pivot['T3-T2'] =pivot['T3_market_signal_queue_pop'] - pivot['T2_market_signal_queue_push']
pivot['T4-T3'] = pivot['T4_order_created'] - pivot['T3_market_signal_queue_pop']
pivot['T5-T4'] = pivot['T5_order_request_queue_pop'] - pivot['T4_order_created']
pivot['T6-T5'] = pivot['T6_onorderevent'] - pivot['T5_order_request_queue_pop']
pivot['latency_total'] = pivot['T6_onorderevent'] - pivot['T1_MD_received']

# --- Step 4: 显示统计信息（单位 微s） ---
stats = pivot[['T2-T1', 'T3-T2','T4-T3', 'T5-T4','T6-T5','latency_total']] / 1e3
print("\n📊 延迟统计信息（微秒）:")
print(stats.describe())



# --- Step 5: 每个延迟阶段分别画在子图中 ---
latencies =  ['T2-T1', 'T3-T2', 'T4-T3', 'T5-T4', 'T6-T5', 'latency_total']
titles = ['T2-T1  行情接收->入队', 'T3-T2  行情入队->出队', 'T4-T3  行情出队->策略完成,订单入队', 'T5-T4  订单入队->出队', 'T6-T5  订单出队到下单', 'latency_total  行情接收到下单']

sns.set(style="whitegrid")
fig, axes = plt.subplots(3, 2, figsize=(14, 10))  # 3 行 2 列子图，共 6 个
axes = axes.flatten()  # 将 axes 展平为一维数组，便于迭代

for i, (latency, title) in enumerate(zip(latencies, titles)):
    sns.kdeplot(stats[latency], fill=True, ax=axes[i], alpha=0.5, linewidth=1.5)
    axes[i].set_title(title, fontproperties=font)  # 设置每个子图的标题使用中文字体
    axes[i].set_xlabel("Latency (微秒)", fontproperties=font)  # 设置x轴标签
    axes[i].set_ylabel("Density", fontproperties=font)  # 设置y轴标签

plt.tight_layout()
plt.savefig("latency_analysis_绑核.png", dpi=300)
plt.close()
