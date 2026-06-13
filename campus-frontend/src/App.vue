<template>
  <div class="h-screen w-full flex bg-slate-50 font-sans text-slate-800">
    
    <aside class="w-[512px] min-w-[512px] flex-shrink-0 bg-white shadow-2xl z-10 flex flex-col border-r border-slate-200">
      
      <div class="p-8 bg-gradient-to-br from-indigo-700 to-indigo-600 text-white flex-shrink-0">
        <h1 class="text-2xl font-bold tracking-widest">校园网络拓扑系统</h1>
        <p class="text-xs text-indigo-200 mt-1.5 opacity-90 font-mono">Algorithm Graph Engine v2.1</p>
      </div>
      
      <div class="flex bg-slate-100 flex-shrink-0 border-b border-slate-200">
        <button v-for="tab in mainTabs" :key="tab.id" @click="activeMainTab = tab.id"
          :class="['flex-1 py-4 text-sm font-bold transition-all', 
                   activeMainTab === tab.id ? 'border-b-4 border-indigo-600 text-indigo-700 bg-white' : 'text-slate-500 hover:bg-slate-200']">
          {{ tab.name }}
        </button>
      </div>
      
      <div class="p-8 flex-1 overflow-y-auto space-y-8 custom-scrollbar">

        <div v-show="activeMainTab === 'edit'" class="space-y-6">
          <!-- 顶点管理卡片 -->
          <div class="bg-slate-50 p-6 border border-slate-200 rounded-xl shadow-sm">
            <h3 class="font-bold text-slate-700 mb-4 flex items-center"><span class="mr-2">🏛️</span> 顶点管理 (Vertex)</h3>
            <div class="flex space-x-2">
              <input v-model="forms.edit.nodeName" placeholder="输入节点名" class="border p-2.5 rounded-lg flex-1 outline-none focus:ring-2 focus:ring-indigo-500" />
              <button @click="editMap('node', 'add')" class="bg-indigo-600 text-white px-5 py-2.5 rounded-lg font-bold hover:bg-indigo-700 transition">添加</button>
              <button @click="editMap('node', 'delete')" class="bg-rose-500 text-white px-5 py-2.5 rounded-lg font-bold hover:bg-rose-600 transition">移除</button>
            </div>
          </div>

          <!-- 边权管理卡片 -->
          <div class="bg-slate-50 p-6 border border-slate-200 rounded-xl shadow-sm">
            <h3 class="font-bold text-slate-700 mb-4 flex items-center"><span class="mr-2">🛤️</span> 边权管理 (Edge Weight)</h3>
            <div class="grid grid-cols-2 gap-4 mb-4">
              <div><span class="text-xs text-slate-500 font-bold mb-1 block">源点 Source</span><select v-model="forms.edit.source" class="w-full border p-2.5 rounded-lg outline-none bg-white"><option v-for="n in mapData.nodes" :value="n.id">{{n.name}}</option></select></div>
              <div><span class="text-xs text-slate-500 font-bold mb-1 block">靶点 Target</span><select v-model="forms.edit.target" class="w-full border p-2.5 rounded-lg outline-none bg-white"><option v-for="n in mapData.nodes" :value="n.id">{{n.name}}</option></select></div>
            </div>
            <div class="flex space-x-2">
              <input type="number" v-model="forms.edit.weight" placeholder="权重" class="border p-2.5 rounded-lg w-24 outline-none" />
              <button @click="editMap('edge', 'add')" class="flex-1 bg-indigo-600 text-white px-4 py-2.5 rounded-lg font-bold hover:bg-indigo-700">写入连线</button>
              <button @click="editMap('edge', 'delete')" class="bg-rose-500 text-white px-4 py-2.5 rounded-lg font-bold hover:bg-rose-600">断开连线</button>
            </div>
          </div>
        
          <!-- 危险操作区卡片（新添加） -->
          <div class="bg-rose-50 p-6 border border-rose-200 rounded-xl shadow-sm">
            <h3 class="font-bold text-rose-800 mb-2 flex items-center"><span class="mr-2">⚠️</span> 危险操作区 (Danger Zone)</h3>
            <p class="text-xs text-rose-600 mb-4 leading-relaxed">
              将清除所有自定义添加的顶点与连线拓扑，强制将网络引擎恢复至系统初始状态 (出厂设置)。
            </p>
            <button @click="resetMap" class="w-full bg-rose-600 text-white font-bold py-2.5 rounded-lg hover:bg-rose-700 transition shadow-lg shadow-rose-200 active:scale-95">
              一键重置系统拓扑
            </button>
          </div>
        </div>

        <div v-show="activeMainTab === 'basic'" class="space-y-6">
          <div class="flex bg-slate-200 p-1.5 rounded-lg">
            <button @click="subModes.basic = 'shortest'" :class="['flex-1 py-2 text-sm font-bold rounded', subModes.basic === 'shortest' ? 'bg-white shadow text-indigo-700' : 'text-slate-500']">Dijkstra 最短路由</button>
            <button @click="subModes.basic = 'n_nodes'" :class="['flex-1 py-2 text-sm font-bold rounded', subModes.basic === 'n_nodes' ? 'bg-white shadow text-indigo-700' : 'text-slate-500']">定长顶点路由</button>
          </div>

          <div class="grid grid-cols-2 gap-4">
            <div><label class="block mb-2 font-bold text-slate-600 text-sm">起点</label><select v-model="forms.basic.source" class="w-full border p-3 rounded-lg bg-white shadow-sm"><option v-for="n in mapData.nodes" :value="n.id">{{n.name}}</option></select></div>
            <div><label class="block mb-2 font-bold text-slate-600 text-sm">终点</label><select v-model="forms.basic.target" class="w-full border p-3 rounded-lg bg-white shadow-sm"><option v-for="n in mapData.nodes" :value="n.id">{{n.name}}</option></select></div>
          </div>
          
          <div v-if="subModes.basic === 'n_nodes'">
            <label class="block mb-2 font-bold text-slate-600 text-sm">确切途经顶点总数 (Length)</label>
            <input type="number" v-model="forms.basic.n" class="w-full border p-3 rounded-lg bg-white shadow-sm" min="2" />
          </div>

          <button @click="calculate('basic')" class="w-full bg-indigo-600 text-white font-bold py-3.5 rounded-xl hover:bg-indigo-700 shadow-lg shadow-indigo-200 transition-all active:scale-95">执行路由推演</button>
        </div>

        <div v-show="activeMainTab === 'advanced'" class="space-y-6">
          <div class="flex bg-slate-200 p-1.5 rounded-lg">
            <button @click="subModes.advanced = 'tour'" :class="['flex-1 py-2 text-sm font-bold rounded', subModes.advanced === 'tour' ? 'bg-white shadow text-indigo-700' : 'text-slate-500']">TSP 最优生成树</button>
            <button @click="subModes.advanced = 'restricted'" :class="['flex-1 py-2 text-sm font-bold rounded', subModes.advanced === 'restricted' ? 'bg-white shadow text-indigo-700' : 'text-slate-500']">前置依赖受限</button>
          </div>

          <div class="bg-indigo-50 p-5 rounded-xl border border-indigo-100 shadow-sm">
            <label class="block mb-3 font-bold text-indigo-900 text-sm">依次点击选择约束顶点</label>
            <div class="grid grid-cols-4 gap-2 mb-4">
              <button v-for="node in mapData.nodes" :key="'adv'+node.id" @click="togglePlace(node.id)"
                :class="['p-2 rounded-lg text-sm font-bold transition-all border', 
                         forms.advanced.places.includes(node.id) ? 'bg-indigo-600 text-white border-indigo-600 shadow' : 'bg-white text-slate-600 hover:border-indigo-400']">
                {{ node.name }}
              </button>
            </div>
            
            <div v-if="forms.advanced.places.length > 0" class="bg-white p-4 rounded-lg border border-indigo-200 text-sm font-mono break-words shadow-sm">
              <span class="block text-indigo-500 font-bold mb-3 border-b border-indigo-100 pb-2">=== 节点角色分配 ===</span>
              
              <div class="flex items-center mb-2">
                <span class="bg-green-100 text-green-700 px-2 py-1 rounded mr-3 font-bold text-xs">🚩 起点</span>
                <span class="font-bold text-slate-700">{{ forms.advanced.places[0] }}</span>
              </div>

              <div v-if="forms.advanced.places.length > 2" class="flex items-start mb-2">
                <span class="bg-amber-100 text-amber-700 px-2 py-1 rounded mr-3 font-bold text-xs whitespace-nowrap">🎯 途经点</span>
                <div class="flex flex-wrap gap-1.5 items-center">
                  <span v-for="p in forms.advanced.places.slice(1, -1)" :key="p" class="bg-slate-100 text-slate-600 px-2 py-0.5 rounded text-xs border border-slate-200 font-bold">
                    {{ p }}
                  </span>
                  <span class="text-xs text-slate-400 ml-1">(由算法统筹排序)</span>
                </div>
              </div>

              <div v-if="forms.advanced.places.length > 1" class="flex items-center mt-2">
                <span class="bg-rose-100 text-rose-700 px-2 py-1 rounded mr-3 font-bold text-xs">🏁 终点</span>
                <span class="font-bold text-slate-700">{{ forms.advanced.places[forms.advanced.places.length - 1] }}</span>
              </div>
            </div>
          </div>

          <div v-if="subModes.advanced === 'restricted'" class="border-t pt-4">
            <label class="block mb-3 font-bold text-slate-700 flex justify-between items-center text-sm">
              设置前置依赖关系
              <button @click="forms.advanced.limits.push({from: '', to: ''})" class="text-xs bg-indigo-100 text-indigo-700 px-3 py-1.5 rounded font-bold hover:bg-indigo-200">+ 新增约束</button>
            </label>
            <div v-for="(limit, idx) in forms.advanced.limits" :key="idx" class="flex items-center space-x-2 mb-3 bg-white p-2 rounded-lg border shadow-sm">
              <select v-model="limit.from" class="border p-2 rounded-lg flex-1 font-bold text-indigo-600 bg-slate-50"><option v-for="n in forms.advanced.places" :value="n">{{n}}</option></select>
              <span class="text-xs font-bold text-slate-400 uppercase tracking-widest">须早于</span>
              <select v-model="limit.to" class="border p-2 rounded-lg flex-1 font-bold text-indigo-600 bg-slate-50"><option v-for="n in forms.advanced.places" :value="n">{{n}}</option></select>
              <button @click="forms.advanced.limits.splice(idx, 1)" class="text-rose-400 hover:text-rose-600 font-bold px-2">✕</button>
            </div>
            <p v-if="forms.advanced.limits.length===0" class="text-xs text-slate-400 mt-2">系统默认：按算法自动寻找连通路径，不强制时间先后依赖。</p>
          </div>

          <button @click="calculate('advanced')" class="w-full bg-indigo-600 text-white font-bold py-3.5 rounded-xl hover:bg-indigo-700 shadow-lg shadow-indigo-200 transition-all active:scale-95">计算约束集拓扑</button>
        </div>

        <div class="mt-8">
          <transition name="fade" mode="out-in">
            <div v-if="results[activeMainTab]?.distance" class="p-6 bg-emerald-50 rounded-2xl border border-emerald-200 shadow-inner">
              <h3 class="text-emerald-800 font-extrabold mb-2 text-lg flex items-center">✅ 路由解析成功</h3>
              <p class="text-sm text-slate-600 mb-4 border-b border-emerald-200 pb-3">全链路总权值: <span class="font-black text-3xl text-emerald-600 ml-2 tracking-tight">{{ results[activeMainTab].distance }}</span></p>
              <div class="text-sm text-slate-800 font-mono bg-white p-4 rounded-xl border border-emerald-100 leading-relaxed shadow-sm">
                <span v-for="(pt, i) in results[activeMainTab].path" :key="i">
                  <span class="font-bold text-emerald-700">{{ pt }}</span>
                  <span v-if="i < results[activeMainTab].path.length - 1" class="text-emerald-300 mx-1.5 font-bold">➜</span>
                </span>
              </div>
            </div>
            <div v-else-if="results[activeMainTab]?.error" class="p-6 bg-rose-50 rounded-2xl border border-rose-200 shadow-inner">
              <h3 class="text-rose-800 font-extrabold mb-2 text-lg flex items-center">⚠️ 路由阻断</h3>
              <p class="text-sm text-rose-600 font-bold">{{ results[activeMainTab].error }}</p>
            </div>
          </transition>
        </div>

      </div>
    </aside>

    <main class="flex-1 min-w-[800px] relative flex flex-col p-6 bg-[radial-gradient(#cbd5e1_1px,transparent_1px)] [background-size:24px_24px]">
      <div class="flex-1 w-full relative bg-white rounded-3xl shadow-2xl border border-slate-200 p-2 overflow-hidden">
         <div ref="chartRef" class="absolute inset-2"></div>
      </div>
    </main>
  </div>
</template>

<script setup>
import { ref, onMounted, reactive, watch } from 'vue';
import axios from 'axios';
import * as echarts from 'echarts';

const API = 'http://localhost:8080/api';
const chartRef = ref(null);
let myChart = null;

const mainTabs = [
  { id: 'edit', name: '🗺️ 拓扑维护' },
  { id: 'basic', name: '📍 经典路由' },
  { id: 'advanced', name: '⭐ 约束规划' }
];
const activeMainTab = ref('basic');
const subModes = reactive({ basic: 'shortest', advanced: 'tour' });

const mapData = reactive({ nodes: [], edges: [] });
const forms = reactive({
  edit: { nodeName: '', source: 'A', target: 'B', weight: 100 },
  basic: { source: 'A', target: 'D', n: 4 },
  advanced: { places: [], limits: [] }
});
const results = reactive({ basic: {}, advanced: {} });

onMounted(async () => {
  myChart = echarts.init(chartRef.value);
  const resizeObserver = new ResizeObserver(() => {
    if (myChart) {
      myChart.resize();
    }
  });
  resizeObserver.observe(chartRef.value);
  await fetchMapData();
});

watch(activeMainTab, (newTab) => renderGraph(results[newTab]?.path || []));

const togglePlace = (nodeId) => {
  const idx = forms.advanced.places.indexOf(nodeId);
  if (idx > -1) forms.advanced.places.splice(idx, 1);
  else forms.advanced.places.push(nodeId);
  
  forms.advanced.limits = forms.advanced.limits.filter(l => 
    forms.advanced.places.includes(l.from) && forms.advanced.places.includes(l.to)
  );
};

const fetchMapData = async () => {
  try {
    const { data } = await axios.get(`${API}/map`);
    const uniqueEdges = []; const edgeSet = new Set();
    data.edges.forEach(e => {
      const k1 = `${e.source}-${e.target}`, k2 = `${e.target}-${e.source}`;
      if (!edgeSet.has(k1) && !edgeSet.has(k2)) { edgeSet.add(k1); uniqueEdges.push(e); }
    });
    mapData.nodes = data.nodes; mapData.edges = uniqueEdges;
    renderGraph(results[activeMainTab.value]?.path || []);
  } catch (e) { alert("接口连接失败"); }
};

const editMap = async (type, action) => {
  try {
    const payload = type === 'node' 
      ? { type, action, node: forms.edit.nodeName }
      : { type, action, source: forms.edit.source, target: forms.edit.target, weight: forms.edit.weight };
    
    const { data } = await axios.post(`${API}/edit`, payload);
    if (data.error) alert(data.error); else await fetchMapData(); 
  } catch (e) { alert("后端异常"); }
};

const resetMap = async () => {
  if (!window.confirm("⚠️ 确定要放弃所有拓扑修改，恢复到默认的校园地图吗？此操作不可逆！")) {
    return;
  }
  
  try {
    const { data } = await axios.post(`${API}/reset`);
    if (data.status === 'success') {
      results.basic = {};
      results.advanced = {};
      forms.advanced.places = [];
      forms.advanced.limits = [];
      
      await fetchMapData(); 
      alert("✅ 系统已恢复至初始拓扑结构！");
    }
  } catch (e) {
    alert("重置请求发送失败，请检查 C++ 后端状态。");
  }
};

const calculate = async (tabCategory) => {
  results[tabCategory] = {}; 
  try {
    let endpoint = tabCategory === 'basic' ? subModes.basic : subModes.advanced;
    const { data } = await axios.post(`${API}/path/${endpoint}`, forms[tabCategory]);
    results[tabCategory] = data;
    renderGraph(data.path || []);
  } catch (error) {
    results[tabCategory] = { error: "路由矩阵算力超限或服务未响应" };
  }
};

const renderGraph = (highlightPath = []) => {
  const chartNodes = mapData.nodes.map(n => ({
    name: n.id,
    symbolSize: highlightPath.includes(n.id) ? 48 : 32,
    itemStyle: {
      color: highlightPath.includes(n.id) ? '#4F46E5' : '#94A3B8', 
      borderColor: '#fff', borderWidth: 3,
      shadowBlur: highlightPath.includes(n.id) ? 20 : 0,
      shadowColor: '#4F46E5'
    },
    label: { show: true, fontSize: 14, color: '#fff', fontWeight: 'bold' }
  }));

  const chartLinks = mapData.edges.map(e => {
    let isPathEdge = false;
    for (let i = 0; i < highlightPath.length - 1; i++) {
      if ((highlightPath[i] === e.source && highlightPath[i+1] === e.target) || 
          (highlightPath[i] === e.target && highlightPath[i+1] === e.source)) {
        isPathEdge = true; break;
      }
    }
    return {
      source: e.source, target: e.target, value: e.weight,
      lineStyle: {
        width: isPathEdge ? 6 : 2,
        color: isPathEdge ? '#4F46E5' : '#E2E8F0',
        type: isPathEdge ? 'solid' : 'dashed'
      },
      label: { show: true, formatter: `{@value}`, fontSize: 12, color: '#94A3B8' }
    };
  });

  const option = {
    tooltip: { formatter: '{b}' },
    animationDurationUpdate: 800,
    animationEasingUpdate: 'cubicOut',
    series: [{
      type: 'graph', layout: 'force',
      force: { repulsion: 1200, edgeLength: [120, 240], gravity: 0.1 },
      roam: true, label: { show: true }, edgeSymbol: ['none', 'none'],
      data: chartNodes, links: chartLinks
    }]
  };
  myChart.setOption(option);
};
</script>

<style>
.fade-enter-active, .fade-leave-active { transition: opacity 0.3s ease, transform 0.3s ease; }
.fade-enter-from, .fade-leave-to { opacity: 0; transform: translateY(10px); }
.custom-scrollbar::-webkit-scrollbar { width: 6px; }
.custom-scrollbar::-webkit-scrollbar-track { background: transparent; }
.custom-scrollbar::-webkit-scrollbar-thumb { background: #CBD5E1; border-radius: 4px; }
.custom-scrollbar::-webkit-scrollbar-thumb:hover { background: #94A3B8; }
</style>