import pickle
import numpy as np
import matplotlib.pyplot as plt
import sys

sys.setrecursionlimit(10**6)

from collections import defaultdict, deque
class Graph_depth:
    def __init__(self):
        # default dictionary to store graph
        self.graph = defaultdict(list)

    # method to add an edge to the graph
    def add_edge(self, u, v):
        self.graph[u].append(v)

    # DFS to calculate maximum depth
    def dfs(self, node, visited, depth):
        visited.add(node)
        max_depth = depth

        # Recur for all the vertices adjacent to this vertex
        for neighbor in self.graph[node]:
            if neighbor not in visited:
                current_depth = self.dfs(neighbor, visited, depth + 1)
                max_depth = max(max_depth, current_depth)

        return max_depth

    # Wrapper function to get the maximum depth of the network
    def get_max_depth(self, start_node):
        visited = set()  # to keep track of visited nodes
        return self.dfs(start_node, visited, 0)
            
class Graph:
    def __init__(self, vertices):
        self.V = vertices
        self.graph = {i: [] for i in range(vertices)}  # Adjacency list

    # Add an edge to the graph
    def add_edge(self, u, v):
        self.graph[u].append(v)
        self.graph[v].append(u)  # Undirected graph

    # DFS to construct the spanning tree
    def dfs_spanning_tree(self, v, visited, parent,edges):
        visited[v] = True

        # Traverse the neighbors of vertex v
        for neighbor in self.graph[v]:
            if not visited[neighbor]:
                # This edge (v, neighbor) is part of the spanning tree
                # print(f"Edge {v} -- {neighbor} is part of the spanning tree")
                edges.append(str(v)+'-'+str(neighbor))
                parent[neighbor] = v
                self.dfs_spanning_tree(neighbor, visited, parent,edges)

    # Function to initiate DFS and construct the spanning tree
    def spanning_tree(self, start_vertex,edges):
        visited = [False] * self.V
        parent = [-1] * self.V  # To track the tree structure

        # print(f"DFS starting from vertex {start_vertex}:")
        self.dfs_spanning_tree(start_vertex, visited, parent,edges)
        
nodes=[2000]
neighbors=[4,6,8,10,12]
mn=[]
class Tree:
    def __init__(self, size):
        self.size = size
        self.adj_matrix = [[0] * size for _ in range(size)]

    def add_edge(self, parent, child):
        self.adj_matrix[parent][child] = 1

# Function to calculate total descendants of a node
def count_descendants(node, adj_matrix):
    def dfs(node):
        count = 0
        for child in range(len(adj_matrix[node])):
            if adj_matrix[node][child] == 1:
                count += 1 + dfs(child)
        return count

    return dfs(node)

# Function to calculate the total number of paths from a node to leaves
def count_paths_to_leaves(node, adj_matrix):
    def dfs(node):
        is_leaf = True
        count = 0
        for child in range(len(adj_matrix[node])):
            if adj_matrix[node][child] == 1:
                is_leaf = False
                count += dfs(child)
        return 1 if is_leaf else count

    return dfs(node)

# Function to find the length of each path from a node to leaves
def find_path_lengths(node, adj_matrix):
    def dfs(node, current_length):
        is_leaf = True
        lengths = []
        for child in range(len(adj_matrix[node])):
            if adj_matrix[node][child] == 1:
                is_leaf = False
                lengths.extend(dfs(child, current_length + 1))
        if is_leaf:
            return [current_length + 1],node
        return lengths

    return dfs(node, 0)

def find_path(start_node, target_node, adj_matrix):
    def dfs(node, path):
        path.append(node)
        if node == target_node:
            return True
        for child in range(len(adj_matrix[node])):
            if adj_matrix[node][child] == 1 and child not in path:
                if dfs(child, path):
                    return True
        path.pop()
        return False

    path = []
    if dfs(start_node, path):
        return path
    else:
        return None

# Function to check if a node is reachable from another node
def is_reachable(start_node, target_node, adj_matrix):
    def dfs(node):
        if node == target_node:
            return True
        visited[node] = True
        for child in range(len(adj_matrix[node])):
            if adj_matrix[node][child] == 1 and not visited[child]:
                if dfs(child):
                    return True
        return False

    visited = [False] * len(adj_matrix)
    return dfs(start_node)

c=32
ecc=16
p=8
rat=8
crc=8
dg=32
tr=1.192*10**-9
tc=4.2686*10**-6
runtime=np.array([0.141666667,0.182,0.189,0.201666667,0.209333333])
r_high=np.array([0.151,0.218,0.203,0.208,0.24])-runtime
r_low=runtime-np.array([0.136,0.139,0.162,0.196,0.189])
r_error=[r_low,r_high]
ps_r=np.array([43.716,45.91883333,64.959,74.15433333,107.3183333])
ps_low=ps_r-np.array([36.08,39.41,36.709,45.09,53.62])
ps_high=np.array([55.94,58.02,66.629,96.77,142.72])-ps_r
ps_error=[ps_low,ps_high]
E_r=np.array([0.2675,0.244366667,0.361786667,0.392233333,0.581533333])
e_low=E_r-np.array([0.196,0.21,0.2067,0.24,0.399])
e_high=np.array([0.399,0.307,0.5679,0.51,0.777])-E_r
e_error=[e_low,e_high]

plt.figure(linewidth=2)
plt.errorbar(np.array(neighbors)+2,runtime,r_error,fmt='-o', marker='*',color='dodgerblue',linewidth=2,capsize=5)
plt.legend(['PISA'], fontsize = 17,ncol=3, loc='upper center', bbox_to_anchor=(0.5, 1.2))
plt.xlabel('Max Number of Neighbors', fontsize = 17,weight='bold')
plt.ylabel('Runtime(ms)', fontsize = 17,weight='bold')
plt.tick_params(axis='both', labelsize=17)
plt.show()
plt.figure(linewidth=2)
plt.errorbar(np.array(neighbors)+2,E_r,e_error, fmt='-o', marker='*',color='dodgerblue',linewidth=2,capsize=5)
plt.legend(['PISA'], fontsize = 17,ncol=3, loc='upper center', bbox_to_anchor=(0.5, 1.2))
plt.xlabel('Max Number of Neighbors',  fontsize = 17,weight='bold')
plt.ylabel('Energy(J)',  fontsize = 17,weight='bold')
plt.tick_params(axis='both', labelsize=17)
plt.show()
plt.figure(linewidth=2)
plt.errorbar(np.array(neighbors)+2,ps_r,ps_error, fmt='-o', marker='*',color='dodgerblue',linewidth=2,capsize=5)
plt.legend(['PISA'], fontsize = 17,ncol=3, loc='upper center', bbox_to_anchor=(0.5, 1.2))
plt.xlabel('Max Number of Neighbors', fontsize = 17,weight='bold')
plt.ylabel('Avg. Pkt. Size (Bytes)', fontsize = 17,weight='bold')
plt.tick_params(axis='both', labelsize=17)
plt.show()

