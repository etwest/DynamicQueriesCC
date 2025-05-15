
#ifndef DYNAMICCC_H_
#define DYNAMICCC_H_

#include <string>
#include <vector>
#include <cstring>
#include <climits>
#include <algorithm>
#include <utility>
#include <cstdio>
#include <cmath>
#include <set>
#include <iostream>
#include <time.h>

#define MAXST 1024
#define MAXDEP 32768

using namespace std;

template <class T> ostream& operator<< (std::ostream& stream, const vector<T> &x) {
	stream << "("; for(size_t i = 0; i < x.size(); ++i) if(i == 0) stream << x[i]; else cout << "," << x[i]; cout << ")"; return stream; }
template <class T1, class T2> ostream& operator<< (std::ostream& stream, const pair<T1,T2> &x) {stream << "(" << x.first << "," << x.second << ")"; return stream;}

class LinkNode {
public:
	int v;
	LinkNode* prev;
	LinkNode* next;
public:
	LinkNode();
	void isolate();
};

class Node {
public:
	//for graph
	vector<int> adj;

	//for tree
	int p; //parent node in the tree
	int sub_cnt; //number of descendants in the tree

	//for union_find
	int f; //father node in union_find
	LinkNode l_start; //start node in the list for union_find
	LinkNode l_end; //end node in the list for union_find

public:
	Node();
	void insert_l_node(LinkNode *v);
	void insert_l_nodes(Node *v);

public:
	vector<pair<int,int> > del_buf;
	vector<pair<int,int> > ins_buf;

public:
	void insert_adj(int u);
	void delete_adj(int u);
	void flush();
};

class DynamicCC {
public:
	static void create_bin(string path);
	static void create_stream(string path, bool is_bipartite = false);
	static bool get_edge(char *line, int &a, int &b, int num_cnt);
	static bool get_temporal_edge(char *line, int &a, int &b, long long &t, int num_cnt);
	static int get_num_cnt(string path);

public:
	int n;
	vector<Node> nodes;
	vector<LinkNode> l_nodes;

public:
	//used in algorithms
	vector<bool> used;
	vector<int> q, l;

public:
	DynamicCC(size_t vertices, bool use_union_find);
	DynamicCC(string path, bool load_graph = true, bool use_union_find = true);
	void init(); //initialize the information in nodes
	bool insert_edge_in_graph(int u, int v);
	bool delete_edge_in_graph(int u, int v);
	int insert_edge(int u, int v); //-1:not inserted; 0:non-tree edge; 1:tree-edge
	int delete_edge(int u, int v); //-1:not inserted; 0:non-tree edge; 1:tree-edge with replacement; 2:tree-edge without replacement

private:
	int insert_edge_balanced(int u, int v);
	int delete_edge_balanced(int u, int v);
	void reroot(int u, int fv);

	bool find_replacement(int u, int f);

public:
	bool use_union_find;

public:
	double avgdelta;
	int maxdelta;
	double avgq;
	int maxq;
	int maxi;
	double avgi;
	int replacesuccessnum;
	int removenum;
	double avg;
	double maxavg;
	int needrerootnum;

public:
	bool query(int u, int v);
	void sample_edges(vector<pair<int,int> > &edges, long long cnt);

private: //for Algorithm_Union_Find
	int get_f(int u);
	void union_f(int u, int v);
	void remove_subtree_union_find(int u, int v,bool needreroot);
};


//=======implementation========

Node::Node(){
	p=-1; f=-1;
	sub_cnt = 0;
	l_start.next = &l_end;
	l_start.prev = NULL;
	l_end.prev = &l_start;
	l_end.next = NULL;
}

void Node::insert_l_node(LinkNode *v) {
	v->next = l_start.next;
	v->prev = &l_start;
	l_start.next->prev = v;
	l_start.next = v;
}

void Node::insert_l_nodes(Node *v) {

	if(v->l_start.next == &v->l_end || v == this) return;

	LinkNode *s = v->l_start.next, *t = v->l_end.prev;
	t->next = l_start.next;
	s->prev = &l_start;
	l_start.next->prev = t;
	l_start.next = s;

	v->l_start.next = &v->l_end;
	v->l_end.prev = &v->l_start;
}


void Node::insert_adj(int u) {
	ins_buf.push_back(make_pair(u,(int)(ins_buf.size()+del_buf.size())));
}
void Node::delete_adj(int u) {
	del_buf.push_back(make_pair(u,(int)(ins_buf.size()+del_buf.size())));
}
void Node::flush() {
	if(ins_buf.size() == 0 && del_buf.size() == 0) return;
	sort(ins_buf.begin(), ins_buf.end());
	sort(del_buf.begin(), del_buf.end());

	int i = 0, d = 0, ni = (int)ins_buf.size(), nd = (int)del_buf.size(), td = 0, ti = 0;

	vector<int> l;
	for(int j = 0; j <= (int) adj.size(); ++j) {
		int v = j < (int) adj.size() ? adj[j] : INT_MAX;
		for(;i < ni && ins_buf[i].first < v;++i) {
			while(i < ni-1 && ins_buf[i].first == ins_buf[i+1].first) ++i;
			while(d < nd && del_buf[d].first < ins_buf[i].first) ++d;
			while(d < nd && del_buf[d].first == ins_buf[i].first) ++d;
			if(d > 0 && del_buf[d-1].first == ins_buf[i].first && del_buf[d-1].second > ins_buf[i].second) continue;
			l.push_back(ins_buf[i].first);
		}
		if(j == (int) adj.size()) break;
		while(d < nd && del_buf[d].first < v) ++d;
		if(d >= nd || del_buf[d].first > v) {
			l.push_back(v);
			while(i < ni && ins_buf[i].first == v) ++i;
			continue;
		}
		if(i < ni && ins_buf[i].first == v) {
			for(; i < ni && ins_buf[i].first == v; ++i) ti=ins_buf[i].second;
			for(; d < nd && del_buf[d].first == v; ++d) td=del_buf[d].second;
			if(ti > td) l.push_back(v);
		}
	}
	ins_buf.clear();
	del_buf.clear();
	adj = l;
}


LinkNode::LinkNode(){v=-1; prev=NULL; next=NULL;}

void LinkNode::isolate() {
	LinkNode *tmp = prev;
	if(prev) {prev->next = next; prev=NULL;}
	if(next) {next->prev = tmp; next=NULL;}
}

DynamicCC::DynamicCC(size_t vertices, bool use_union_find) {
	this->avgdelta = 0;
	this->avgi = 0;
	this->avgq = 0;
	this->maxdelta = 0;
	this->maxq = 0;
	this->maxi = 0;
	this->removenum = 0;
	this->replacesuccessnum = 0;
	this->avg = 0;
	this->maxavg = 0;
	this->needrerootnum = 0;
	this->use_union_find = use_union_find;

	n = vertices;
	nodes.resize(n);
}

DynamicCC::DynamicCC(string path, bool load_graph, bool use_union_find) {
	printf( "path = %s, load_graph = %s, use_union_find = %s\n", path.c_str(), (load_graph?"true":"false"), (use_union_find?"true":"false") );
	this->avgdelta = 0;
	this->avgi = 0;
	this->avgq = 0;
	this->maxdelta = 0;
	this->maxq = 0;
	this->maxi = 0;
	this->removenum = 0;
	this->replacesuccessnum = 0;
	this->avg = 0;
	this->maxavg = 0;
	this->needrerootnum = 0;
	this->use_union_find = use_union_find;
	FILE* fin = fopen( (path+"graph.bin").c_str(), "rb" );

	if(fin == NULL || !load_graph) {
		if(fin) fclose(fin);
		fin = fopen( (path+"graph.stream").c_str(), "rb" );
		fread(&n, sizeof(int), 1, fin);
		nodes.resize(n);
		fclose(fin);
		return;
	}


	fread( &n, sizeof(int), 1, fin );
	nodes.resize(n);
	int *deg = new int[n], *dat = new int[n];

	printf( "Loading graph...\n" );
	long long m = 0;
	fread( deg, sizeof(int), n, fin );
	for(int i = 0; i < n; ++i) {
		fread(dat, sizeof(int), deg[i], fin);
		m += deg[i];
		nodes[i].adj.assign(dat, dat+deg[i]);
	}

	delete[] deg; delete[] dat;
	fclose(fin);

	this->use_union_find = use_union_find;
	printf( "Graph loaded, n = %d, m = %lld\n", n, m/2 );
}

void DynamicCC::init() {
	vector<pair<int,int> > s;
	used.resize(n, false);
	for(int vid=0; vid<n; ++vid) {
		nodes[vid].flush();
		int len = nodes[vid].adj.size();
		s.push_back(make_pair(len, -vid));
	}
	sort(s.begin(),s.end());
	vector<int> q;

	if(use_union_find) {
		l_nodes.resize(n);

		for(int v=0; v<n; ++v) {
			l_nodes[v].v = v;
			l_nodes[v].prev = NULL;
			l_nodes[v].next = NULL;
			nodes[v].f = v;
			nodes[v].l_start.next = &nodes[v].l_end;
			nodes[v].l_end.prev = &nodes[v].l_start;
			nodes[v].l_start.prev = NULL;
			nodes[v].l_end.next = NULL;
		}
	}

	for(int v=0; v<n; ++v) {
		nodes[v].p = -1;
		nodes[v].sub_cnt = 1;
	}


	for(int i=n-1; i>=0; --i) {
		int f = -s[i].second;

		if(used[f]) continue;
		q.clear();

		used[f] = true;
		q.push_back(f);

		if(use_union_find) nodes[f].insert_l_node(&l_nodes[f]);

		for(int s=0; s < (int)q.size(); ++s) {
			int p = q[s];
			for(int j = 0; j < (int) nodes[p].adj.size(); ++j) {
				int v = nodes[p].adj[j];
				if(!used[v]) {
					used[v] = true;
					q.push_back(v);
					nodes[v].p = p;

					if(use_union_find) {
						nodes[v].f = f;
						nodes[f].insert_l_node(&l_nodes[v]);
					}
				}
			}
		}

		for(int i = (int)q.size()-1; i>0; --i)
			nodes[nodes[q[i]].p].sub_cnt += nodes[q[i]].sub_cnt;

		int r = -1, ss = (int) q.size()/2;
		for(int i = (int)q.size()-1; i>=0; --i)
			if(r == -1 && nodes[q[i]].sub_cnt > ss) r = q[i];
		if(r != f) reroot(r, f);
	}

	used.clear();
	used.resize(n, false);
}

bool DynamicCC::query(int u, int v) {
	if(u < 0 || u >= n || v < 0 || v >= n) return false;
	if(use_union_find) return get_f(u) == get_f(v);
	while(nodes[u].p != -1) u = nodes[u].p;
	while(nodes[v].p != -1) v = nodes[v].p;
	return u == v;
}

int DynamicCC::get_f(int u) {
	if(nodes[u].f != u) {
		int f = get_f(nodes[u].f);
		if(nodes[u].f != f) {
			nodes[u].f = f;
			l_nodes[u].isolate();
			nodes[f].insert_l_node(&l_nodes[u]);
		}
	}
	return nodes[u].f;
}

void DynamicCC::union_f(int fu, int fv) { //fu->fv
	if(fu == fv) return;

	nodes[fu].f = fv;
	l_nodes[fu].isolate();
	nodes[fv].insert_l_node(&l_nodes[fu]);
}

void DynamicCC::reroot(int u, int f) {
	int p, pp;
	for(p = nodes[u].p, nodes[u].p = -1; p != -1;) pp=nodes[p].p, nodes[p].p = u, u = p, p = pp;
	for(p = nodes[u].p; p != -1; u = p, p = nodes[p].p) nodes[u].sub_cnt -= nodes[p].sub_cnt, nodes[p].sub_cnt += nodes[u].sub_cnt;

	if(use_union_find && f >= 0) {
		nodes[f].f = u;
		l_nodes[f].isolate();
		nodes[u].insert_l_node(&l_nodes[f]);

		nodes[u].f = u;
		l_nodes[u].isolate();
		nodes[u].insert_l_node(&l_nodes[u]);
	}
}

int DynamicCC::insert_edge_balanced(int u, int v) {
	int fu, fv, p, pp, d;

	if(!use_union_find) {
		for(fu = u; nodes[fu].p != -1;) {
			fu = nodes[fu].p;
		}
		for(fv = v; nodes[fv].p != -1;) {
			fv = nodes[fv].p;
		}
	} else {
		fu = get_f(u); fv = get_f(v);
	}

	if(fu == fv) {
		bool reshape = false;
		for(d = 0, p = nodes[u].p, pp = nodes[v].p; d < MAXDEP; p = nodes[p].p, pp = nodes[pp].p, ++d)
			if(p == -1){ if(pp != -1 && nodes[pp].p != -1) {reshape = true; swap(u,v); swap(p,pp); } break;}
			else if(pp == -1) { if(p != -1 && nodes[p].p != -1) reshape = true; break;}
		if(reshape) {
			int dlt = 0;
			for(; p != -1; p = nodes[p].p) ++dlt;
			for(dlt = dlt/2-1, p = u; dlt > 0; --dlt) p = nodes[p].p;

			for(pp = nodes[p].p; pp != -1; pp = nodes[pp].p) nodes[pp].sub_cnt -= nodes[p].sub_cnt;

			nodes[p].p = -1;
			reroot(u,-1);
			//for(p = nodes[u].p; p != -1; p = nodes[p].p) nodes[p].sub_cnt -= nodes[u].sub_cnt;

			nodes[u].p = v;

			int s = (nodes[fu].sub_cnt + nodes[u].sub_cnt)/2, r = -1;
			for(p = v; p != -1; p = nodes[p].p) {
				nodes[p].sub_cnt += nodes[u].sub_cnt;
				if(r == -1 && nodes[p].sub_cnt > s) r = p;
			}
			if(r != fu) reroot(r, fu);
		}
		return 0;
	}
	if(nodes[fu].sub_cnt > nodes[fv].sub_cnt) {swap(u,v); swap(fu,fv);}
	
	for(p = nodes[u].p, nodes[u].p = v; p != -1;) pp=nodes[p].p, nodes[p].p = u, u = p, p = pp;

	int s = (nodes[fu].sub_cnt + nodes[fv].sub_cnt)/2, r = -1;

	
	for(p = v; p != -1; p = nodes[p].p) {
		nodes[p].sub_cnt += nodes[fu].sub_cnt;
		if(r == -1 && nodes[p].sub_cnt > s) r = p;
	}

	for(p = nodes[u].p; p != v; u = p, p = nodes[p].p) nodes[u].sub_cnt -= nodes[p].sub_cnt, nodes[p].sub_cnt += nodes[u].sub_cnt;

	if(use_union_find) union_f(fu,fv);
	if(r != fv) reroot(r, fv);
	return 1;
}

int DynamicCC::delete_edge_balanced(int u, int v) {
	if(nodes[u].p != v && nodes[v].p != u) return 0;
	if(nodes[v].p == u) swap(u,v); //make u->v

	int f;

	for(int w = v; w != -1; w = nodes[w].p) nodes[w].sub_cnt -= nodes[u].sub_cnt, f = w;
	nodes[u].p = -1;
	int ns, nl;
	bool needreroot;
	if(nodes[u].sub_cnt>nodes[f].sub_cnt){
		ns = f;
		nl = u;
		needreroot = true;
	}	
	else
	{
		ns = u;
		nl = f;
		needreroot = false;
	}

	if(use_union_find&&needreroot) {
		nodes[f].f = u;
		l_nodes[f].isolate();
		nodes[u].insert_l_node(&l_nodes[f]);

		nodes[u].f = u;
		l_nodes[u].isolate();
		nodes[u].insert_l_node(&l_nodes[u]);
		needrerootnum++;
	}
	
	if(find_replacement(ns,nl)) return 1;

	if(use_union_find) remove_subtree_union_find(ns,nl,needreroot);

	return 2;
}


bool DynamicCC::find_replacement(int u, int f) {
	q.clear(); l.clear();
	q.push_back(u);
	l.push_back(u);
	used[u] = true;

	for(int i = 0; i < (int) q.size(); ++i) {
		int x = q[i], p, pp;
		nodes[x].flush();
		for(int j = 0; j < (int) nodes[x].adj.size(); ++j) {
			int y = nodes[x].adj[j];
			if(y == nodes[x].p) continue;
			if(nodes[y].p == x) {
				q.push_back(y);
				if(!used[y]) {used[y]=true; l.push_back(y);}
				continue;
			}
			bool succ = true;
			for(int w = y; w != -1; w = nodes[w].p) {
				if(used[w]) {succ = false; break;}
				used[w] = true;
				l.push_back(w);
			}
			if(!succ) continue;

			for(p = nodes[x].p, nodes[x].p = y; p != -1;) pp=nodes[p].p, nodes[p].p = x, x = p, p = pp;

			int s = (nodes[f].sub_cnt + nodes[u].sub_cnt)/2, r = -1;
			for(p = y; p != -1; p = nodes[p].p) {
				nodes[p].sub_cnt += nodes[u].sub_cnt;
				if(r == -1 && nodes[p].sub_cnt > s) r = p;
			}

			for(p = nodes[x].p; p != y; x = p, p = nodes[p].p) nodes[x].sub_cnt -= nodes[p].sub_cnt, nodes[p].sub_cnt += nodes[x].sub_cnt;
			for(int k = 0; k < (int) l.size(); ++k) used[l[k]] = false;

			if(r != f) reroot(r, f);
			avgi = avgi + i + 1;
			if(i + 1>maxi)
				maxi = i + 1;
			replacesuccessnum++;
			return true;
		}
	}
	for(int k = 0; k < (int) l.size(); ++k) used[l[k]] = false;
	return false;
}

void DynamicCC::remove_subtree_union_find(int u, int v,bool needreroot) {
	
	removenum++;
	avgq += (int)q.size();
	if((int)q.size()>maxq)
		maxq = (int)q.size();
	int dnum = 0;
	
	int fv = v;
	for(int i = 0; i < (int) q.size(); ++i) {
		int x = q[i];
		if(nodes[x].l_start.next != &nodes[x].l_end){
			for(LinkNode *y = nodes[x].l_start.next; y != &nodes[x].l_end; y = y->next)
				nodes[y->v].f = fv,dnum++;
			nodes[fv].insert_l_nodes(&nodes[x]);
		}
	}

	avgdelta += dnum;
	avg = avg + (double)dnum / (double)q.size();
	if((double)dnum / (double)q.size()>maxavg)
		maxavg = (double)dnum / (double)q.size();
	if (dnum > maxdelta)
		maxdelta = dnum;

	for(int i = 0; i < (int) q.size(); ++i) {
		int x = q[i];
		l_nodes[x].isolate();
		nodes[u].insert_l_node(&l_nodes[x]);
		nodes[x].f = u;
	}
}


int DynamicCC::insert_edge(int u, int v) {
	if(!insert_edge_in_graph(u,v)) return -1;
	return insert_edge_balanced(u,v);
}

int DynamicCC::delete_edge(int u, int v) {
	if(!delete_edge_in_graph(u,v)) return -1;
	return delete_edge_balanced(u,v);
}

bool DynamicCC::insert_edge_in_graph(int u, int v) {
	if(u < 0 || u >= n || v < 0 || v >= n || u == v) return false;
	nodes[u].insert_adj(v);
	nodes[v].insert_adj(u);
	return true;
}

bool DynamicCC::delete_edge_in_graph(int u, int v) {
	if(u < 0 || u >= n || v < 0 || v >= n || u == v) return false;
	nodes[u].delete_adj(v);
	nodes[v].delete_adj(u);
	return true;
}

void DynamicCC::sample_edges(vector<pair<int,int> > &edges, long long cnt) {
	long long m = 0;
	for(int v = 0; v < n; ++v) m += nodes[v].adj.size();
	long long now_cnt = 0;
	long long dlt = max((m/2)/cnt,(long long)1);
	edges.clear();
	for(int u = 0; u < n && edges.size() < cnt; ++u) {
		for(int i = 0; i < (int) nodes[u].adj.size(); ++i) {
			int v = nodes[u].adj[i];
			if(v < u) continue;
			if(++now_cnt == dlt && edges.size() < cnt) {
				now_cnt = 0;
				edges.push_back(make_pair(u,v));
				if(edges.size() >= cnt) break;
			}
		}
	}
	int s = (int) edges.size();
	for(int i = 0; i < s; ++i) swap(edges[i], edges[rand()%s]);
}

bool DynamicCC::get_edge(char *line, int &a, int &b, int num_cnt) {
	if( !isdigit(line[0]) ) return false;
	vector<char*> v_num;
	int len = (int) strlen(line);
	for( int i = 0; i < len; ++i )
		if( !isdigit(line[i]) && line[i] != '.') line[i] = '\0';
		else if(i == 0 || !line[i-1]) v_num.push_back(line+i);
	if( (int) v_num.size() != num_cnt ) return false;
	sscanf( v_num[0], "%d", &a );
	sscanf( v_num[1], "%d", &b );
	return true;
}

bool DynamicCC::get_temporal_edge(char *line, int &a, int &b, long long &t, int num_cnt) {
	if( !isdigit(line[0]) ) return false;
	vector<char*> v_num;
	int len = (int) strlen(line);
	for( int i = 0; i < len; ++i )
		if( !isdigit(line[i]) && line[i] != '.') line[i] = '\0';
		else if(i == 0 || !line[i-1]) v_num.push_back(line+i);
	if( (int) v_num.size() != num_cnt ) return false;
	sscanf( v_num[0], "%d", &a );
	sscanf( v_num[1], "%d", &b );
	sscanf( v_num[num_cnt-1], "%lld", &t );
	return true;
}

int DynamicCC::get_num_cnt(string path) {
	FILE *fin = fopen( (path + "graph.txt").c_str(), "r" );
	char line[MAXST];
	int cnt = 0, min_cnt = 100;

	while(fgets( line, MAXST, fin ) && cnt < 10) {
		if(!isdigit(line[0])) continue;
		vector<char*> v_num;
		int len = (int) strlen(line);
		for(int i = 0; i < len; ++i)
			if(!isdigit(line[i]) && line[i] != '.') line[i] = '\0';
			else if(i == 0 || !line[i-1]) v_num.push_back(line+i);
		if( (int) v_num.size() < 2 ) continue;
		min_cnt = min(min_cnt, (int) v_num.size());
		++cnt;
	}
	fclose(fin);
	return min_cnt;
}

void DynamicCC::create_bin(string path) {
	FILE *fin = fopen((path + "graph.txt").c_str(), "r" );
	char line[MAXST];
	int n = 0, a, b, num_cnt = get_num_cnt(path);
	vector< pair<int,int> > el;
	long long cnt = 0, m = 0;

	printf("Loading text, num_cnt = %d...\n", num_cnt);
	while(fgets( line, MAXST, fin)) {
		if(!get_edge(line, a, b, num_cnt)) continue;
		if(a < 0 || b < 0 || a == b) continue;
		el.push_back(make_pair(a, b));
		n = max(max(n, a+1), b+1);
		if((++cnt) % (long long) 10000000 == 0) printf("%lld lines finished\n", cnt);
	}
	fclose(fin);

	vector<int> *con = new vector<int>[n];
	printf("Deduplicating...\n");

	for(size_t i = 0; i < el.size(); ++i) {
		con[el[i].first].push_back(el[i].second);
		con[el[i].second].push_back(el[i].first);
	}

	for(int i = 0; i < n; ++i)
		if(con[i].size() > 0){
			sort(con[i].begin(), con[i].end());
			int p = 1;
			for(int j = 1; j < (int) con[i].size(); ++j)
				if(con[i][j-1] != con[i][j]) con[i][p++] = con[i][j];
			con[i].resize( p ); m += p;
		}

	int *dat = new int[m], *deg = new int[n], **adj = new int *[n];

	long long pos = 0;
	for(int i = 0; i < n; ++i) {
		adj[i] = dat + pos;
		pos += (int)con[i].size();
	}
	memset(deg, 0, sizeof(int) * n);

	for(int i = 0; i < n; ++i)
		for(int p = 0; p < (int) con[i].size(); ++p)
			adj[i][deg[i]++] = con[i][p];

	printf( "Saving binary...\n" );

	FILE *fout = fopen((path + "graph.bin").c_str(), "wb");
	fwrite(&n, sizeof(int), 1, fout);
	fwrite(deg, sizeof(int), n, fout);
	fwrite(dat, sizeof(int), m, fout);

	fclose(fout);
	printf("Created binary file, n = %d, m = %lld\n", n, m/2);

	delete[] adj; delete[] deg; delete[] dat; delete[] con;
}

void DynamicCC::create_stream(string path, bool is_bipartite) {
	FILE *fin = fopen((path + "graph.txt").c_str(), "r" );
	char line[MAXST];
	int n = 0, n1 = 0, n2 = 0, a, b, num_cnt = get_num_cnt(path);
	vector<pair<long long, pair<int,int> > > el;
	long long m = 0, t;

	printf("Loading text, num_cnt = %d, is_bipartite = %s...\n", num_cnt, is_bipartite?"true":"false");
	while(fgets( line, MAXST, fin)) {
		if(!get_temporal_edge(line, a, b, t, num_cnt)) continue;
		if(a < 0 || b < 0) continue;
		el.push_back(make_pair(t,make_pair(a, b)));
		n = max(max(n, a+1), b+1);
		n1 = max(n1, a+1);
		n2 = max(n2, b+1);
		if((++m) % (long long) 10000000 == 0) printf("%lld lines finished\n", m);
	}
	fclose(fin);

	if(is_bipartite) {
		printf( "n1 = %d, n2 = %d\n", n1, n2 );
		n = n1 + n2;
		for(long long i = 0; i < (long long) el.size(); ++i) el[i].second.second += n1;
	}

	for(long long i = 0; i < (long long) el.size(); ++i)
		if(el[i].second.first > el[i].second.second)
			swap(el[i].second.first, el[i].second.second);

	sort(el.begin(),el.end());

	m = 0;
	for(long long i = 0; i < (long long) el.size(); ++i)
		if(i == 0 || el[i] != el[i-1])
			el[m++] = el[i];

	FILE *fout = fopen((path + "graph.stream").c_str(), "wb");
	fwrite(&n, sizeof(int), 1, fout);
	fwrite(&m, sizeof(long long), 1, fout);
	fwrite(el.data(), sizeof(pair<long long,pair<int,int> >), m, fout);

	fclose(fout);
	printf("Created stream file, n = %d, m = %lld\n", n, m);
}

#endif /* DYNAMICCC_H_ */
