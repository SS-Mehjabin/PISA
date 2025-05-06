import numpy as np
import random
from collections import defaultdict, deque
from itertools import combinations
import networkx as nx 
import matplotlib.pyplot as plt 
import pickle
from tqdm import tqdm
n=15
from collections import defaultdict

# Class to represent a graph
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
    
class Graph_dij():

	def __init__(self, vertices):
		self.V = vertices
		self.graph = [[0 for column in range(vertices)]
					for row in range(vertices)]

	def printSolution(self, dist):
# 		print("Vertex \t Distance from Source")
		x=[]
		for node in range(self.V):
# 			print(node, "\t\t", dist[node])
			x.append(dist[node])
		return x
	# A utility function to find the vertex with
	# minimum distance value, from the set of vertices
	# not yet included in shortest path tree
	def minDistance(self, dist, sptSet):

		# Initialize minimum distance for next node
		min = 1e7

		# Search not nearest vertex not in the
		# shortest path tree
		for v in range(self.V):
			if dist[v] < min and sptSet[v] == False:
				min = dist[v]
				min_index = v

		return min_index

	# Function that implements Dijkstra's single source
	# shortest path algorithm for a graph represented
	# using adjacency matrix representation
	def dijkstra(self, src):

		dist = [1e7] * self.V
		dist[src] = 0
		sptSet = [False] * self.V

		for cout in range(self.V):

			# Pick the minimum distance vertex from
			# the set of vertices not yet processed.
			# u is always equal to src in first iteration
			u = self.minDistance(dist, sptSet)

			# Put the minimum distance vertex in the
			# shortest path tree
			sptSet[u] = True

			# Update dist value of the adjacent vertices
			# of the picked vertex only if the current
			# distance is greater than new distance and
			# the vertex in not in the shortest path tree
			for v in range(self.V):
				if (self.graph[u][v] > 0 and
				sptSet[v] == False and
				dist[v] > dist[u] + self.graph[u][v]):
					dist[v] = dist[u] + self.graph[u][v]

		x=self.printSolution(dist)
		return x


class Graph_cut:
    def __init__(self, vertices):
        self.V = vertices  # Number of vertices in the graph
        self.graph = defaultdict(list)  # Adjacency list representation
        self.capacity = defaultdict(lambda: defaultdict(int))  # Capacity of edges

    # Add edge to the graph with a given capacity
    def add_edge(self, u, v, capacity):
        self.graph[u].append(v)
        self.graph[v].append(u)  # Since this is an undirected graph (can be modified if directed)
        self.capacity[u][v] = capacity
        self.capacity[v][u] = capacity  # Add reverse capacity for undirected graph

    # Breadth-First Search to find augmenting path in residual graph
    def bfs(self, source, sink, parent):
        visited = [False] * self.V
        queue = deque([source])
        visited[source] = True

        while queue:
            u = queue.popleft()

            for v in self.graph[u]:
                # If not visited and there's capacity left in the edge
                if not visited[v] and self.capacity[u][v] > 0:
                    parent[v] = u
                    if v == sink:
                        return True
                    queue.append(v)
                    visited[v] = True
        return False

    # Implementation of the Edmonds-Karp (Ford-Fulkerson) algorithm to find max flow
    def edmonds_karp(self, source, sink):
        parent = [-1] * self.V  # Stores the path
        max_flow = 0  # Stores the total maximum flow

        while self.bfs(source, sink, parent):
            # Find the bottleneck flow in the augmenting path
            path_flow = float('Inf')
            s = sink
            while s != source:
                path_flow = min(path_flow, self.capacity[parent[s]][s])
                s = parent[s]

            # Update the capacities in the residual graph
            v = sink
            while v != source:
                u = parent[v]
                self.capacity[u][v] -= path_flow
                self.capacity[v][u] += path_flow
                v = parent[v]

            # Add path flow to overall flow
            max_flow += path_flow

        return max_flow

    # Perform a DFS to find all reachable vertices after finding max flow
    def dfs(self, source, visited):
        visited[source] = True
        for i in self.graph[source]:
            if not visited[i] and self.capacity[source][i] > 0:
                self.dfs(i, visited)

    # Function to find the minimum cut after calculating max flow
    def min_cut(self, source, sink):
        # First, compute the max flow using Edmonds-Karp algorithm
        self.edmonds_karp(source, sink)

        # Perform DFS to find all reachable vertices from source in the residual graph
        visited = [False] * self.V
        self.dfs(source, visited)

        # Find edges that cross from reachable to non-reachable vertices, which form the min-cut
        min_cut_edges = []
        for u in range(self.V):
            for v in self.graph[u]:
                if visited[u] and not visited[v] and self.capacity[u][v] == 0:
                    min_cut_edges.append((u, v))

        return min_cut_edges

def dfs1(graph, node, visited, current_path, best_path):
    # Mark the current node as visited
    visited[node] = True
    # Add the current node to the current path
    current_path.append(node)
    
    # If the current path is longer than the best found so far, update the best path
    if len(current_path) > len(best_path):
        best_path[:] = current_path[:]
    
    # Explore all neighbors
    if node in graph:
        for neighbor in graph[node]:
            if not visited[neighbor]:
                dfs1(graph, neighbor, visited, current_path, best_path)
    
    # Backtrack: remove the node from the current path and mark it as unvisited
    current_path.pop()
    visited[node] = False

def find_longest_path_from_node(graph, start_node):
    best_path = []
    
    # Initialize visited dictionary for all nodes in the graph
    visited = {node: False for node in graph}
    
    # Start DFS from the given starting node
    current_path = []
    dfs1(graph, start_node, visited, current_path, best_path)
    
    return best_path

def BFS_SP(graph, start, goal):
    explored = []
     
    # Queue for traversing the 
    # graph in the BFS
    queue = [[start]]
     
    # If the desired node is 
    # reached
    if start == goal:
        print("Same Node")
        return
     
    # Loop to traverse the graph 
    # with the help of the queue
    while queue:
        path = queue.pop(0)
        node = path[-1]
         
        # Condition to check if the
        # current node is not visited
        if node not in explored:
            neighbours = graph[node]
             
            # Loop to iterate over the 
            # neighbours of the node
            for neighbour in neighbours:
                new_path = list(path)
                new_path.append(neighbour)
                queue.append(new_path)
                 
                # Condition to check if the 
                # neighbour node is the goal
                if neighbour == goal:
                    # print("Shortest path = ", *new_path)
                    return list(new_path)
            explored.append(node)
 
    # Condition when the nodes 
    # are not connected
    print("So sorry, but a connecting"\
                "path doesn't exist :(")
    return

def bfs_reachable_nodes(graph, start_node):
    # Set to store all reachable nodes
    reachable = set()
    
    # Initialize a queue and enqueue the start node
    queue = deque([start_node])
    
    # Add the start node to reachable set
    reachable.add(start_node)
    
    # BFS to explore the graph
    while queue:
        current_node = queue.popleft()
        
        # Explore all neighbors of the current node
        if current_node in graph:
            for neighbor in graph[current_node]:
                if neighbor not in reachable:  # If the neighbor hasn't been visited
                    reachable.add(neighbor)    # Add to reachable set
                    queue.append(neighbor)     # Enqueue the neighbor
    
    return reachable

def intersection(lst1, lst2):
    lst3 = [value for value in lst1 if value in lst2]
    return lst3

# Function to calculate the edge-cut of the current partition
def calculate_cut_size(graph, partition):
    cut_size = 0
    for u, v in graph.edges():
        if partition[u] != partition[v]:
            cut_size += 1
    return cut_size

# Function to balance the partitions
def balance_partitions(partition, n_parts, nodes_per_part):
    part_sizes = defaultdict(int)
    balanced_partition = partition.copy()
    
    # Ensure that no partition exceeds the balanced size
    for node, p in partition.items():
        if part_sizes[p] < nodes_per_part:
            part_sizes[p] += 1
        else:
            # Move the node to a partition that needs more nodes
            for new_p in range(n_parts):
                if part_sizes[new_p] < nodes_per_part:
                    balanced_partition[node] = new_p
                    part_sizes[new_p] += 1
                    break
    return balanced_partition

# Kernighan-Lin algorithm for n partitions
def kernighan_lin_n_part(graph, n_parts, max_iter=1):
    nodes = list(graph.nodes())
    num_nodes = len(nodes)
    nodes_per_part = num_nodes // n_parts
    
    # Initial random partition
    partition = {node: i % n_parts for i, node in enumerate(nodes)}
    random.shuffle(nodes)
    
    # Main loop of the algorithm
    for _ in range(max_iter):
        initial_cut_size = calculate_cut_size(graph, partition)
        
        # Attempt to swap nodes between partitions to reduce the cut size
        for u in tqdm(nodes):
            for v in tqdm(nodes):
                if u != v and partition[u] != partition[v]:
                    # Swap u and v and calculate the new cut size
                    partition[u], partition[v] = partition[v], partition[u]
                    new_cut_size = calculate_cut_size(graph, partition)
                    
                    # If the swap reduces the cut size, keep it, otherwise revert
                    if new_cut_size < initial_cut_size:
                        initial_cut_size = new_cut_size
                    else:
                        partition[u], partition[v] = partition[v], partition[u]

        # Balance the partitions to ensure no partition is too large
        partition = balance_partitions(partition, n_parts, nodes_per_part)
    
    # Final partitions
    final_partitions = defaultdict(list)
    for node, p in partition.items():
        final_partitions[p].append(node)
    
    return final_partitions

g = Graph_dij(n)


# print(graph)

def pro1(graph):
    adj=np.zeros((n,n))
    
    for key,values in graph.items():
        for value in values:
            adj[key][value]=1
            adj[value][key]=1
    
    nxn=np.zeros((n,n))
    covered=[]
    pp=1
    queue = [0]
    visited = []
    node_list=list(range(n))
    while(1):
        # print(queue)
        ll=len(queue)
        for i in range(ll):
            x=queue[0]
            queue.remove(queue[0])
            if x not in visited:
                visited.append(x)
                for j in graph[x]:
                    if j not in visited:
                        queue.append(j)
                        nxn[x][j]=pp
        pp=pp+1
        if set(visited)==set(node_list):
            break
    td=0
    for i in range(n):
        for j in range(n):
            if nxn[i][j]>td:
                td=nxn[i][j]*1
    
    g.graph=adj*1
    # g_c = Graph_cut(n)
    # graph_dict = {}
    # for i in range(1,n):
    #     graph_dict[i]=[]
    #     for j in range(1,n):
    #         if adj[i][j]==1:
    #             g_c.add_edge(i, j, n-nxn[i][j])
    #             graph_dict[i].append(j)
    
    g_c = nx.DiGraph()
    for i in range(n):
        g_c.add_node(i)
        
    graph_dict = {}
    for i in range(1,n):
        graph_dict[i]=[]
        for j in range(1,n):
            if adj[i][j]==1:
                g_c.add_edge(i, j, capacity=(n-nxn[i][j]))
                graph_dict[i].append(j)
    
    neigh=[]
    for i in range(n):
        if adj[0][i]==1:
            neigh.append(i)
    
    # print(neigh)
    comb = list(combinations(neigh, 2))
    cuts=[]
    
    def find_cutset(graph,source,sink):
        cut_value, partition = nx.minimum_cut(graph, source, sink)
        reachable, non_reachable = partition
        cutset = set()
        for u,nbrs in ((n,graph[n]) for n in reachable):
            cutset.update((u,v) for v in nbrs if v in non_reachable)
        return cutset
        
    for i in comb:
        start=i[0]
        stop=i[1]
        
        if start>stop:
            source=stop
            sink=start
        else:
            source=start
            sink=stop
        
        # Find the minimum cut that disconnects source and sink
        # min_cut_edges = g_c.min_cut(source, sink)
        min_cut_edges = find_cutset(g_c,source, sink)
        # print("Edges in the minimum cut:", min_cut_edges)
        for j in min_cut_edges:
            if j not in cuts:
                cuts.append(j)
    
    for i in cuts:
        if i[1] in graph_dict[i[0]]:
            graph_dict[i[0]].remove(i[1])
        if i[0] in graph_dict[i[1]]:
            graph_dict[i[1]].remove(i[0])
    
    # print(graph_dict)
    node_list=np.array(range(n))
    nodes_covered=[]
    nodes_covered.append(0)
    attestation_paths=[]
    partitions={}
    path_partitions={}
    for i in neigh:
        partitions[i]=[0]
        partitions[i].extend(list(bfs_reachable_nodes(graph_dict, i)))
        start_node=i
        longest_path = find_longest_path_from_node(graph_dict, start_node)
        # print("Longest Path from node", start_node, ":", longest_path)
        nodes_covered.extend(longest_path)
        path=[0]
        path.extend(longest_path)
        attestation_paths.append(path)
        a=[0]
        a.extend(longest_path)
        path_partitions[i]=a.copy()
    
    nodes_not_covered=[]
    for i in node_list:
        if i not in nodes_covered:
            nodes_not_covered.append(i)
    
    for i in nodes_not_covered:
        x=graph[i]
        flag=0
        for j in x:
            if i in graph_dict[j]:
                graph_dict[j].remove(i)
                graph_dict[i].remove(j)
        for j in neigh:
            fl=0
            if i not in partitions[j]:
                # print(i)
                # print(partitions[j])
                for k in partitions[j]:
                    if adj[i][k]==1:
                        graph_dict[k].append(i)
                        graph_dict[i].append(k)
                        fl=1
            # print(fl)
            if fl==1:
                longest_path = find_longest_path_from_node(graph_dict, j)
                # print(longest_path)
                a=[0]
                a.extend(longest_path)
                # print(path_partitions[j])
                # print(a)
                flag=0
                if i not in a:
                    flag=1
                    break
                for k in a:
                    if ((k not in path_partitions[j])&(k!=i)):
                        flag=1
                        break
                if flag==0:
                    # print('blah')
                    nodes_not_covered.remove(i)
                    path_partitions[j]=a.copy()
                    for xx in partitions:
                        if i in partitions[xx]:
                            partitions[xx].remove(i)
                    partitions[j]=a.copy()
                    break
    
    # print(nodes_not_covered)
    if len(nodes_not_covered)!=0:
        for i in nodes_not_covered:
            path_partitions[i]=BFS_SP(graph, 0, i)
            # print('blah')
    return nodes_not_covered,partitions,path_partitions,td


def pro3(graph):
    adj=np.zeros((n,n))

    for key,values in graph.items():
        for value in values:
            adj[key][value]=1
            adj[value][key]=1
            
    neigh=[]
    for i in range(n):
        if adj[0][i]==1:
            neigh.append(i)


    G=nx.DiGraph()
    for i in range(1,n):
        for j in range(1,n):
            if ((i not in neigh) & (j not in neigh)):
                if adj[i][j]==1:
                    G.add_edge(i,j)

    n_parts = len(neigh)
    partitions_prelim = kernighan_lin_n_part(G, n_parts)
    print("Partitions Done")
    nodes_list=list(range(1,n))
    for i in neigh:
        nodes_list.remove(i)
    # Output the partitions_prelim
    incl=[]
    for part, nodes in partitions_prelim.items():
        # print(f"Partition {part + 1}: {nodes}")
        incl.extend(nodes)

    rem=[]

    for i in nodes_list:
        # print(i)
        if i not in incl:
            rem.append(i)
    # print(rem)

    dn=[]
    for part, nodes in partitions_prelim.items():
        for i in neigh:
            flag=0
            if i not in dn:
                for node in nodes:
                    if ((adj[i][node]==1)|(adj[node][i]==1)):
                        flag=1
                        dn.append(i)
                        partitions_prelim[part].append(i)
                        break
                if flag==1:
                    break
                
    for part, nodes in partitions_prelim.items():
        for i in rem:
            flag=0
            if i not in dn:
                for node in nodes:
                    if ((adj[i][node]==1)|(adj[node][i]==1)):
                        flag=1
                        dn.append(i)
                        partitions_prelim[part].append(i)
                        break
                if flag==1:
                    break

    # for part, nodes in partitions_prelim.items():
    #     print(f"Partition {part + 1}: {nodes}")
        
    graph_dict={}
    for i in range(n):
        graph_dict[i]=[]
    for i in partitions_prelim:
        for j in partitions_prelim[i]:
            # graph_dict[j]=[]
            for k in partitions_prelim[i]:
                if adj[j][k]==1:
                    graph_dict[j].append(k)

    partitions={}
    for i in neigh:
        partitions[i]=[0]



    for i in partitions_prelim:
        for j in neigh:
            if j in partitions_prelim[i]:
                partitions[j].extend(partitions_prelim[i])

    node_list=np.array(range(n))
    nodes_covered=[]
    nodes_covered.append(0)
    attestation_paths=[]
    partitions={}
    path_partitions={}
    for i in tqdm(neigh):
        partitions[i]=[0]
        partitions[i].extend(list(bfs_reachable_nodes(graph_dict, i)))
        ggr={}
        # print(partitions[i])
        # print(graph_dict)
        for xz in partitions[i]:
            ggr[xz]=[0]
            for zx in partitions[i]:
                if ((xz!=zx)&(xz!=0)):
                    if zx in graph_dict[xz]:
                        ggr[xz].append(zx)
        
        start_node=i
        print('Len GGR=')
        print(len(ggr))
        # print(ggr)
        longest_path = find_longest_path_from_node(ggr, start_node)
        # print("Longest Path from node", start_node, ":", longest_path)
        nodes_covered.extend(longest_path)
        path=[0]
        path.extend(longest_path)
        attestation_paths.append(path)
        a=[0]
        a.extend(longest_path)
        path_partitions[i]=a.copy()

    nodes_not_covered=[]
    for i in node_list:
        if i not in nodes_covered:
            nodes_not_covered.append(i)

    for i in tqdm(nodes_not_covered):
        x=graph[i]
        flag=0
        for j in x:
            if j in graph_dict:
                if i in graph_dict[j]:
                    graph_dict[j].remove(i)
                    graph_dict[i].remove(j)
                    
        for j in tqdm(neigh):
            fl=0
            if i not in partitions[j]:
                # print(i)
                # print(partitions[j])
                for k in partitions[j]:
                    if adj[i][k]==1:
                        if k not in graph_dict:
                            graph_dict[k]=[]
                        if i not in graph_dict:
                            graph_dict[i]=[]
                        graph_dict[k].append(i)
                        graph_dict[i].append(k)
                        fl=1
            # print(fl)
            if fl==1:
                partitions[j]=[0]
                partitions[j].extend(list(bfs_reachable_nodes(graph_dict, j)))
                ggr={}
                # print(partitions[i])
                # print(graph_dict)
                for xz in partitions[j]:
                    ggr[xz]=[0]
                    for zx in partitions[j]:
                        if ((xz!=zx)&(xz!=0)):
                            if zx in graph_dict[xz]:
                                ggr[xz].append(zx)
                
                start_node=j
                print('Len GGR=')
                print(len(ggr))
                # print(ggr)
                longest_path = find_longest_path_from_node(ggr, j)
                # print(longest_path)
                a=[0]
                a.extend(longest_path)
                # print(path_partitions[j])
                # print(a)
                flag=0
                if i not in a:
                    flag=1
                    break
                for k in a:
                    if ((k not in path_partitions[j])&(k!=i)):
                        flag=1
                        break
                if flag==0:
                    # print('blah')
                    nodes_not_covered.remove(i)
                    path_partitions[j]=a.copy()
                    for xx in partitions:
                        if i in partitions[xx]:
                            partitions[xx].remove(i)
                    partitions[j]=a.copy()
                    break
                else:
                    for j in x:
                        if j in graph_dict:
                            if i in graph_dict[j]:
                                graph_dict[j].remove(i)
                                graph_dict[i].remove(j)
                    

    # print(nodes_not_covered)
    if len(nodes_not_covered)!=0:
        for i in nodes_not_covered:
            path_partitions[i]=BFS_SP(graph, 0, i)
            # print('blah')
            
    return nodes_not_covered,partitions,path_partitions

min_cut_depths=[]
min_cut_degrees=[]
min_cut_branches=[]
KL_depths=[]
KL_degrees=[]
KL_branches=[]
total_depth=[]



# graph = {
#     0: [1,2,3],
#     1: [4],
#     4: [1,6],
#     5: [6],
#     6: [4,5,7],
#     2: [7,8],
#     7: [6,2,8],
#     8: [2,7,9],
#     9: [8],
#     3: [10,11],
#     10: [3,12,11],
#     11: [15,3,12,10],
#     12: [13,10,11],
#     13: [12,14],
#     14: [13],
#     15: [11]
# }

# graph = {
#     0: [7,1,2,8],
#     1: [0,3,4,5,12],
#     2: [0,3,6,9,13],
#     3: [1,2,4,5,9],
#     4: [1,3,6],
#     5: [1,3,8,11],
#     6: [2,4,7,10],
#     7: [0,6],
#     8: [0,5,11,13,14],
#     9: [2,3,10,14],
#     10: [6,9],
#     11: [5,8,12],
#     12: [1,11],
#     13: [2,8],
#     14: [8,9]
# }

# graph = {
#     0: [1,7,4,8,3,2],
#     1: [0,13,10,4],
#     2: [0,3,5,9,11,13],
#     3: [2,0,6,8,14],
#     4: [0,6,1,5],
#     5: [2,4,14],
#     6: [3,4,7],
#     7: [0,6],
#     8: [3,0],
#     9: [2,12],
#     10: [1],
#     11: [2],
#     12: [9],
#     13: [2,1],
#     14: [3,5]
# }

iterations=[500,1000,2000,3000,4000,5000]
neighbors=[4,6,8,10,12]

for n in iterations:
    for neigh in neighbors:
        for iterp in range(1):
            print(n)
            gg=Graph_depth()
            while(1):
                # Create empty directed graph
                G = nx.DiGraph()
                edges=[]
            
                # Add nodes
                for i in range(n):
                    G.add_node(i)
            
                
                graph={}
                # Add edges (randomly for now)
                for i in range (n):
                    graph[i]=[]
            
                for i in range(n):
                    if i==0:
                        r=int(n/20)
                        # r=3
                    else:
                        r = random.randint(1, neigh) # Number of nodes i will be connected to
                    rr=[]
                    # abc=0
                    # for xx in range(n):
                    #     if xx!=i:
                    #         if i in graph[xx]:
                    #             abc=abc+1
                    
                    
                    # if abc<r:
                    #     r=r-abc
                    # else:
                    #     r=0
                    for j in range(r):
                        rrr = random.randint(0, n-1)
                        if rrr != i:
                            rr.append(rrr) # Node i will be connected to 
                    rr=list(set(rr))
                    for j in rr:
                        G.add_edge(i, j)
                        graph[i].append(j)
                        gg.add_edge(i,j)
                        G.add_edge(j, i)
                        graph[j].append(i)
                        gg.add_edge(j,i)
                        edges.append((i,j))
                pp=[]
                for i in range (n):
                    graph[i]=list(set(graph[i]))
                    pp.append(len(graph[i]))
                if 0 not in pp:
                    break
                else:
                    print('Damn')
                
            print(len(edges))
            with open('graph_'+str(n)+'_'+str(neigh)+'.pkl', 'wb') as f:
                pickle.dump(graph, f)
            # total_depth.append(gg.get_max_depth(0)) 
            maxn=n*1
            for i in range(1):
                na,pa,ppa=pro3(graph)
                print(ppa)
                if maxn>len(na):
                    n3=na.copy()
                    p3=pa.copy()
                    pp3=ppa.copy()
                    maxn=len(na)
            
            
            
            # print("KL:\n")
            adj=np.zeros((n,n))
            
            for key,values in pp3.items():
                for value in range(len(values)-1):
                    adj[values[value]][values[value+1]]=1
                    adj[values[value+1]][values[value]]=1
            
            mm=0
            xx=0
            for i in range(1,n):
                if mm<sum(adj[i]):
                    mm=sum(adj[i])
                if sum(adj[i])>2:
                    xx=xx+1
            # print("Number of nodes with branches= ")
            # print(xx)
            KL_branches.append(xx)
            # print("\n")
            # print("Maximum Node Degree= ")
            # print(mm)
            KL_degrees.append(mm)
            # print("\n")
            mm=0
            for i in pp3:
                if mm<len(pp3[i]):
                    mm=len(pp3[i])
            # print("Maximum Depth= ")
            # print(mm-1)
            KL_depths.append(mm-1)
            # print("\n")
            # print(pp3)

    # print(min_cut_depths)
    # print(min_cut_degrees)
    # print(min_cut_branches)
    # print(KL_depths)
    # print(KL_degrees)
    # print(KL_branches)
    # print(total_depth)
        with open('paths_'+str(n)+'_'+str(neigh)+'.pkl', 'wb') as f:
            pickle.dump(pp3, f)
# f=open('data.csv','w')
# f.write("Number of Nodes,Depth of Network,Depth for Min-cut,Node Degree for Min-cut,Number of nodes with branches for Min_cut,Depth for KL,Node Degree for KL,Number of nodes with branches for KL\n")
# for n in range(len(iterations)):
    # for iterp in range(1):
        # f.write(str(int(iterations[n])))
        # f.write(',')
        # f.write(str(int(total_depth[n+iterp])))
        # f.write(',')
        # # f.write(str(int(min_cut_depths[n+iterp])))
        # # f.write(',')
        # # f.write(str(int(min_cut_degrees[n+iterp])))
        # # f.write(',')
        # # f.write(str(int(min_cut_branches[n+iterp])))
        # # f.write(',')
        # f.write(str(int(KL_depths[n+iterp])))
        # f.write(',')
        # f.write(str(int(KL_degrees[n+iterp])))
        # f.write(',')
        # f.write(str(int(KL_branches[n+iterp])))
        # f.write('\n')
# f.close()