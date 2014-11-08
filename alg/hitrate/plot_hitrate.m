tmp = load("2q.txt");
x = tmp(:, 1);
tq = tmp(:, 2);
arc = load("arc.txt")(:, 2);
fifo = load("fifo.txt")(:, 2);
lfu = load("lfu.txt")(:, 2);
lru = load("lru.txt")(:, 2);
mid = load("mid.txt")(:, 2);
mq = load("mq.txt")(:, 2);
mq_wo_exp = load("mq_wo_exp.txt")(:, 2);
s4lru = load("s4lru.txt")(:, 2);

plot(x, mq_wo_exp, 'linewidth', 2,
     x, mid, 'linewidth', 2,
     x, s4lru, 'linewidth', 2,
     x, lfu, 'linewidth', 2,
     x, arc, 'linewidth', 2,
     x, tq, 'linewidth', 2,
     x, mq, 'linewidth', 2,
     x, lru, '-.', 'linewidth', 2,
     x, fifo, '-.', 'linewidth', 2);

l = legend('MQ*','Mid LRU','S4LRU','LFU','ARC','2Q',
       'MQ','LRU', 'FIFO',
       'location', 'east');
 
set(l, 'fontsize', 14);

grid on;

xlabel('Cache size in item number');
ylabel('Hit rate in %');

print('-dpng', '-color', 'hitrate.png');
