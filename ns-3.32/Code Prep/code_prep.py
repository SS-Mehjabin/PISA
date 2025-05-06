import pickle

with open('pos.pkl', 'rb') as f:
    pos = pickle.load(f)
    
f=open('part1.txt','r')
lines=f.readlines()
f.close()
w=open('attestation.cc','w')
n=1000
for line in lines:
    w.write(line.replace("n_nodes",str(n)))
w.write('\n')

for i in range(n):
    w.write('  not_malicious.Add(c.Get('+str(i)+'));\n')

f=open('part2.txt','r')
lines=f.readlines()
f.close()
for line in lines:
    w.write(line)
w.write('\n')

for i in range(n):
    w.write('  positionAlloc->Add (Vector ('+str(pos[i][0])+', '+str(pos[i][1])+', 0.0));\n')
    
f=open('part3.txt','r')
lines=f.readlines()
f.close()
for line in lines:
    w.write(line)
w.write('\n')

for i in range(n):
    w.write('  Ptr<Socket> recvSink'+str(i)+' = Socket::CreateSocket (c.Get ('+str(i)+'), tid);\n')
    w.write('  InetSocketAddress local'+str(i)+' = InetSocketAddress (Ipv4Address::GetAny (), 80);\n')
    w.write('  recvSink'+str(i)+'->Bind (local'+str(i)+');\n')
    w.write('  recvSink'+str(i)+'->SetRecvCallback (MakeCallback (&ReceivePacket));\n')
 
    
with open('paths.pkl', 'rb') as f:
    path = pickle.load(f)
    
pp=0
end_nodes=[]
for i in path:
    if path[i][len(path[i])-1]!=0:
        j=path[i][len(path[i])-1]
    else:
        j=path[i][len(path[i])-2]
    end_nodes.append(j)
    w.write('  Ptr<Socket> source'+str(pp)+' = Socket::CreateSocket (c.Get (0), tid);\n')
    w.write('  InetSocketAddress remote'+str(pp)+' = InetSocketAddress (interfaces.GetAddress ('+str(j)+', 0), 80);\n')
    w.write('  source'+str(pp)+'->Connect (remote'+str(pp)+');\n')
    pp=pp+1
    
pp=0
for key,values in path.items():
    past_value=values[1]*1
    for value in values[2:]:
        if value!=0:
            w.write('  Ptr<Ipv4StaticRouting> staticRoute'+str(pp)+' = staticRouting.GetStaticRouting(c.Get('+str(value)+')->GetObject<Ipv4>());\n')
            w.write('  staticRoute'+str(pp)+'->AddHostRouteTo(interfaces.GetAddress(0), interfaces.GetAddress('+str(past_value)+'), 1, 0);\n')
            pp=pp+1
            past_value=value*1
            
for key,values in path.items():
    past_value=values[1]*1
    for value in values[2:]:
        if value!=0:
            w.write('  Ptr<Ipv4StaticRouting> staticRoute'+str(pp)+' = staticRouting.GetStaticRouting(c.Get(0)->GetObject<Ipv4>());\n')
            w.write('  staticRoute'+str(pp)+'->AddHostRouteTo(interfaces.GetAddress('+str(value)+'), interfaces.GetAddress('+str(past_value)+'), 1, 0);\n')
            pp=pp+1
            past_value=value*1

w.write('  if (tracing == true)\n')
w.write('    {\n')
w.write('      wifiPhy.EnablePcap ("selective", devices);\n')
w.write('    }\n')

for i in range(len(path)):
    w.write('  Simulator::Schedule (Seconds ('+str(i*10+2)+'.0), &GenerateTraffic,source'+str(i))
    w.write(', packetSize, numPackets, interPacketInterval);\n\n')

f=open('part4.txt','r')
lines=f.readlines()
f.close()
for line in lines:
    w.write(line)
w.write('\n')