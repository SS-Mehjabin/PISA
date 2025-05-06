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
        
nodes=[5000]
neighbors=[4]
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
for neigh in nodes:
    runtime=np.zeros(len(neighbors))
    esdra=np.zeros(len(neighbors))
    seed=np.zeros(len(neighbors))
    seda=np.zeros(len(neighbors))
    ps_r=np.zeros(len(neighbors))
    ps_salad=np.zeros(len(neighbors))
    ps_seed=np.zeros(len(neighbors))
    ps_seda=np.zeros(len(neighbors))
    E_r=np.zeros(len(neighbors))
    E_salad=np.zeros(len(neighbors))
    E_seed=np.zeros(len(neighbors))
    E_seda=np.zeros(len(neighbors))
    pq=0
    for node in neighbors:
        with open('paths_'+str(neigh)+'_'+str(node)+'.pkl', 'rb') as f:
            paths1 = pickle.load(f)
        paths={}
        covered=[]
        for key,values in paths1.items():
            if values[len(values)-1]==0:
                paths[key]=values[0:len(values)-1]
            else:
                paths[key]=values
        
        tree = Tree(neigh)
        for path in paths:
            for x in range(len(paths[path])-1):
                if(paths[path][x+1] not in covered):
                    tree.add_edge(paths[path][x], paths[path][x+1])
                    covered.append(paths[path][x+1])
        initiators=[]
        # leaves=[]
        for ini in range(len(tree.adj_matrix[0])):
            if tree.adj_matrix[0][ini]==1:
                initiators.append(ini)
        # for n in range(node):
        #     if sum(tree.adj_matrix[n])==0:
        #         leaves.append(n)
        tot_time=0
        for ini in initiators:
            # print(count_descendants(ini,tree.adj_matrix))
            # print(count_paths_to_leaves(ini,tree.adj_matrix))
            x=find_path_lengths(ini, tree.adj_matrix)
            path_lengths=[]
            leaves=[]
            # print(x[2])
            for xx in range(len(x)):
                if xx%2==0:
                    path_lengths.extend(x[xx])
                else:
                    leaves.append(x[xx])
            pos=path_lengths.index(max(path_lengths))
            leaf=leaves[pos]
            hops=max(path_lengths)
            
            pt=find_path(ini, leaf, tree.adj_matrix)
            data_sent=0
            for r in pt:
                x=find_path_lengths(r, tree.adj_matrix)
                path_lengths=[]
                leaves=[]
                # print(x[2])
                for xx in range(len(x)):
                    if xx%2==0:
                        path_lengths.extend(x[xx])
                    else:
                        leaves.append(x[xx])
                num_des=count_descendants(r,tree.adj_matrix)
                num_paths=count_paths_to_leaves(r,tree.adj_matrix)
                lengths=sum(path_lengths)
                data_sent=data_sent+c+num_des*(p+ecc+rat)+num_paths*(c)+lengths*(ecc)+(lengths-num_paths)*(p)
                
            data_received=0
            for r in range(len(pt)):
                num_des=count_descendants(pt[r],tree.adj_matrix)
                predecessor=r
                data_received=data_received+num_des*(crc)+predecessor*(ecc)+dg
            
            tt=((data_received+data_sent)*tr+2*hops*tc+(2*hops+1)*(17*10**-9))*1000
            if tt>tot_time:
                tot_time=tt*1
        runtime[pq]=tot_time*1
        
        packets_sent=0
        packets_received=0
        for ini in range(1,neigh):
            print(ini)
            x=find_path_lengths(ini, tree.adj_matrix)
            path_lengths=[]
            leaves=[]
            # print(x[2])
            for xx in range(len(x)):
                if xx%2==0:
                    path_lengths.extend(x[xx])
                else:
                    leaves.append(x[xx])
            num_des=count_descendants(r,tree.adj_matrix)
            num_paths=count_paths_to_leaves(r,tree.adj_matrix)
            lengths=sum(path_lengths)
            packets_sent=packets_sent+c+num_des*(p+ecc+rat)+num_paths*(c)+lengths*(ecc)+(lengths-num_paths)*(p)
            if (find_path(0, ini, tree.adj_matrix)):
                pt=len(find_path(0, ini, tree.adj_matrix))
            else:
                pt=1
            packets_received=packets_received+num_des*(crc)+pt*(ecc)+dg
        
        ps_r[pq]=((packets_received+packets_sent)/8)/neigh
        E_r[pq]=(packets_received*0.36+packets_sent*0.32+0.0001*neigh)/1000000
        
        # print(runtime)    
        
        with open('graph_'+str(neigh)+'_'+str(node)+'.pkl', 'rb') as f:
            paths = pickle.load(f)
        
        g=Graph_depth()
        edges=[]
        for key,values in paths.items():
            x=values[0]*1
            for value in values[1:]:
                g.add_edge(x, value)
                x=value*1
        # g.spanning_tree(0,edges)
        
        # gg={}
        # for j in edges:
        #     i=j.split('-')
        #     if i[0] in gg:
        #         gg[int(i[0])].append(int(i[1]))
        #     else:
        #         gg[int(i[0])]=[int(i[1])]
                
            
        # g1=Graph_depth()    
        # for key,values in gg.items():
        #     for value in values:
        #         g1.add_edge(key, value)
                
        ht=g.get_max_depth(0)
        # prop=ht/node
        print(ht)
        n=500
        m=node*1
        # if ht>n:
        #     ht=n
        tr=1.192*10**-9
        tc=4.2686*10**-6
        esdra[pq]=(((132+36*neigh)*ht+m*ht)*8*tr+(1+ht)*tc+(m+3*m*ht)*400*10**-9+2500*10**-9)*1000
        ps_salad[pq]=((132+36*neigh)*ht+m*ht)/(ht)
        E_salad[pq]=((100*m)*0.32+((36*neigh+32)*m)*0.36+2*m*0.016)*neigh/1000000
        
        seda[pq]=((280+168*ht+ht*m)*8*tr+(1+ht)*tc+(2+4*ht+ht*m)*400*10**-9+(1+ht)*2500*10**-9)*1000
        ps_seda[pq]=(280+168*ht+ht*m)/(ht)
        E_seda[pq]=((80+104*m)*.32+(104+80*m)*.36+(3+3*m)*0.016)*neigh/1000000
        
        seed[pq]=((320+160*ht)*8*tr+(1+ht)*tc+(1+2*ht)*400*10**-9+(1+ht)*2500*10**-9)*1000
        ps_seed[pq]=(320+160*ht)/(ht)
        E_seed[pq]=(160*0.32+(160*m)*0.36+.016)*neigh/1000000
        # print(pp)
        # print(esdra[pp])
        
        pq=pq+1
    plt.figure(linewidth=2)
    plt.plot(np.array(neighbors)+2,runtime, linestyle='--', marker='*',color='dodgerblue',linewidth=2)
    # plt.plot(nodes,esdra, linestyle='--', marker='x',color='violet',linewidth=2)
    plt.plot(np.array(neighbors)+2,seda, linestyle='--', marker='o',color='forestgreen',linewidth=2)
    plt.plot(np.array(neighbors)+2,seed, linestyle='--', marker='^',color='orange',linewidth=2)
    plt.legend(['PISA','SEDA','SeED'], fontsize = 17)
    # plt.title('Runtime for Average Number of neighbors='+str(neigh))
    plt.xlabel('Max Number of Neighbors', fontsize = 17,weight='bold')
    plt.ylabel('Runtime(ms)', fontsize = 17,weight='bold')
    plt.tick_params(axis='both', labelsize=17)
    # plt.savefig("runtimevsnodes.png", dpi=500)
    plt.show()
    plt.figure(linewidth=2)
    plt.plot(np.array(neighbors)+2,E_r, linestyle='--', marker='*',color='dodgerblue',linewidth=2)
    # plt.plot(nodes,E_salad, linestyle='--', marker='x',color='violet')
    plt.plot(np.array(neighbors)+2,E_seda, linestyle='--', marker='o',color='forestgreen',linewidth=2)
    plt.plot(np.array(neighbors)+2,E_seed, linestyle='--', marker='^',color='orange',linewidth=2)
    plt.legend(['PISA','SEDA','SeED'], fontsize = 17)
    # plt.title('Runtime for Average Number of neighbors='+str(neigh))
    plt.xlabel('Max Number of Neighbors',  fontsize = 17,weight='bold')
    plt.ylabel('Energy(J)',  fontsize = 17,weight='bold')
    plt.tick_params(axis='both', labelsize=17)
    # plt.savefig("energy.png", dpi=500)
    # plt.ylim([0,15])
    plt.show()
    plt.figure(linewidth=2)
    plt.plot(np.array(neighbors)+2,ps_r, linestyle='--', marker='*',color='dodgerblue',linewidth=2)
    # plt.plot(nodes,ps_salad, linestyle='--', marker='x',color='violet')
    plt.plot(np.array(neighbors)+2,ps_seda, linestyle='--', marker='o',color='forestgreen',linewidth=2)
    plt.plot(np.array(neighbors)+2,ps_seed, linestyle='--', marker='^',color='orange',linewidth=2)
    plt.legend(['PISA','SEDA','SeED'], fontsize = 17)
    # plt.title('Runtime for Average Number of neighbors='+str(neigh))
    plt.xlabel('Max Number of Neighbors', fontsize = 17,weight='bold')
    plt.ylabel('Avg. Pkt. Size (Bytes)', fontsize = 17,weight='bold')
    plt.tick_params(axis='both', labelsize=17)
    # plt.savefig("packet_size.png", dpi=500)
    # plt.ylim([0,15])
    plt.show()

