/*
 * $Id: main.cpp,v 1.8 2005/02/16 10:45:39 tsuruoka Exp $
 */

#include <stdio.h>
#include <fstream>
#include <map>
#include <list>
#include <iostream>
#include <sstream>
#include "maxent.h"
#include "common.h"

using namespace std;

bool IGNORE_DISTINCTION_BETWEEN_NNP_AND_NN = false;

const int NUM_TRAIN_SENTENCES = 99999999;
//const int NUM_TRAIN_SENTENCES = 10000;
//const int NUM_TRAIN_SENTENCES = 2000;

const int NUM_TEST_SENTENCES = 99999999;

//ME_Sample mesample(const vector<Token> &vt, int i, const string & prepos, const string & prepos2);
ME_Sample mesample(const vector<Token> &vt, int i, const string & prepos);
void viterbi(vector<Token> & vt, const ME_Model & me);
string postag(const string & s, const ME_Model & me);
string bidir_postag(const string & s, const vector<ME_Model> & vme);

void
bidir_postagging(vector<Sentence> & vs,
                 const multimap<string, string> & tagdic,
                 const vector<ME_Model> & vme);

int
bidir_train(const vector<Sentence> & vs, int para);

int train(const vector<Sentence> & vs, ME_Model & me)
{
  vector<ME_Sample> train_samples;

  cerr << "extracting features...";
  int n = 0;
  for (vector<Sentence>::const_iterator i = vs.begin(); i != vs.end(); i++) {
    const Sentence & s = *i;
    for (int j = 0; j < s.size(); j++) {
      string prepos = "BOS";
      if (j > 0) prepos = s[j-1].pos;
      //      string prepos2 = "BOS";
      //      if (j > 1) prepos = s[j-2].pos;
      //      train_samples.push_back(mesample(s, j, prepos, prepos2));
      train_samples.push_back(mesample(s, j, prepos));
    }
  }
  cerr << "done" << endl;

  //  me.learn(train_samples, 1000, 0, 500.0);
  //  me.learn(train_samples, 1000, 3, 500.0);
  //  me.learn(train_samples, 10000, 0, 0, 0.1);
  //  me.set_heldout(10000, 0);
  me.set_heldout(1000, 0);
  me.train(train_samples, 0, 1000);
  //  me.train(train_samples, 0, 0, 1.0);
}

void deterministic(vector<Token> & vt, const ME_Model & me)
{
  for (int i = 0; i < vt.size(); i++) {
    string prepos = "BOS";
    if (i > 0) prepos = vt[i-1].prd;
    //    string prepos2 = "BOS";
    //    if (i > 1) prepos2 = vt[i-2].prd;
    //    ME_Sample mes = mesample(vt, i, prepos, prepos2);
    ME_Sample mes = mesample(vt, i, prepos);
    //    vector<double> membp(me.num_classes());
    //    vt[i].prd = me.classify(mes, &membp);
    vector<double> membp = me.classify(mes);
    vt[i].prd = mes.label;
  }
}

void postagging(vector<Sentence> & vs, const ME_Model & me)
{
  int num_classes = me.num_classes();

  cerr << "now tagging";
  int n = 0;
  for (vector<Sentence>::iterator i = vs.begin(); i != vs.end(); i++) {
    Sentence & s = *i;
    //    bidir_decode_deterministic(s, me);
    //    viterbi(s, me);
    deterministic(s, me);
    if (n++ % 10 == 0) cerr << ".";
  }
  cerr << endl;
}

void evaluate(const vector<Sentence> & vs)
{
  int ncorrect = 0;
  int ntotal = 0;
  for (vector<Sentence>::const_iterator i = vs.begin(); i != vs.end(); i++) {
    const Sentence & s = *i;
    for (int j = 0; j < s.size(); j++) {
      ntotal++;
      string pos = s[j].pos;
      string prd = s[j].prd;
      if (IGNORE_DISTINCTION_BETWEEN_NNP_AND_NN) {      
        if (pos == "NNP") pos = "NN";
        if (pos == "NNPS") pos = "NNS";
        if (prd == "NNP") prd = "NN";
        if (prd == "NNPS") prd = "NNS";
      }
      if (pos == prd) ncorrect++;
      //      cout << s[j].str << "\t" << s[j].pos << "\t" << s[j].prd << endl;
    }
  }
  fprintf(stderr, "%d / %d = %f\n", ncorrect, ntotal, (double)ncorrect / ntotal);
}

void load_tag_dictionary(const string & filename, multimap<string, string> & tagdic)
{
  tagdic.clear();
  ifstream ifile(filename.c_str());
  if (!ifile) {
    cerr << "cannot open " << filename << endl;
    exit(1);
  }
  string line;
  while (getline(ifile, line)) {
    istringstream is(line.c_str());
    string str, tag;
    is >> str >> tag;
    tagdic.insert(pair<string, string>(str, tag));
  }
  ifile.close();
}

void make_tag_dictionary(const vector<Sentence> & vs)
{
  multimap<string, string> wt;
  for (vector<Sentence>::const_iterator i = vs.begin(); i != vs.end(); i++) {
    const Sentence & s = *i;
    for (int j = 0; j < s.size(); j++) {
      //      wt[s[j].str] = s[j].pos;
      bool found = false;
      for (multimap<string, string>::const_iterator k = wt.lower_bound(s[j].str);
           k !=  wt.upper_bound(s[j].str); k++) {
        if (k->second ==  s[j].pos) { found = true; break; }
      }
      if (found) continue;
      wt.insert(pair<string, string>(s[j].str, s[j].pos));
    }
  }
  for (multimap<string, string>::const_iterator i = wt.begin(); i != wt.end(); i++) {
    cout << i->first << '\t' << i->second << endl;
  }

}

void read_pos(const string & filename, vector<Sentence> & vs, int num_sentences = -1)
{
  ifstream ifile(filename.c_str());

  string line;
  int n = 0;
  cerr << "reading " << filename;
  while (getline(ifile,line)) {
    istringstream is(line);
    string s;
    Sentence sentence;
    while (is >> s) {
      string::size_type i = s.find_last_of('/');
      string str = s.substr(0, i);
      string pos = s.substr(i+1);
      if (str == "-LRB-" && pos == "-LRB") { str = "("; pos = "("; }
      if (str == "-RRB-" && pos == "-RRB") { str = ")"; pos = ")"; }
      if (str == "-LSB-" && pos == "-LSB") { str = "["; pos = "["; }
      if (str == "-RSB-" && pos == "-RSB") { str = "]"; pos = "]"; }
      if (str == "-LCB-" && pos == "-LCB") { str = "{"; pos = "{"; }
      if (str == "-RCB-" && pos == "-RCB") { str = "}"; pos = "}"; }
      Token t(str, pos);
      sentence.push_back(t);
    }
    vs.push_back(sentence);
    if (vs.size() >= num_sentences) break;
    if (n++ % 10000 == 0) cerr << ".";
  }
  cerr << endl;

  ifile.close();
}


int main(int argc, char** argv)
{
  int para = -1;
  if (argc == 2) para = atoi(argv[1]);

  vector<ME_Model> vme(16);

  for (int i = 0; i < 16; i++) {
    char buf[1000];
    sprintf(buf, "./models_medline/model.bidir.%d", i);
    cerr << "loading " << string(buf) << endl;
    vme[i].load_from_file(buf);
  }

  string line;
  while (getline(cin, line)) {
    string postagged = bidir_postag(line, vme);
    cout << postagged << endl;
  }
  
}

/*
 * $Log: main.cpp,v $
 * Revision 1.8  2005/02/16 10:45:39  tsuruoka
 * acl05 submit
 *
 * Revision 1.7  2005/01/10 13:44:36  tsuruoka
 * a
 *
 * Revision 1.6  2004/12/30 10:34:56  tsuruoka
 * add bidir_decode_search
 *
 * Revision 1.5  2004/12/25 10:47:12  tsuruoka
 * add load_tag_dictionary()
 *
 * Revision 1.4  2004/12/25 09:22:57  tsuruoka
 * add make_tag_dictionary()
 *
 * Revision 1.3  2004/12/21 13:54:46  tsuruoka
 * add bidir.cpp
 *
 * Revision 1.2  2004/12/20 12:06:24  tsuruoka
 * change the data
 *
 * Revision 1.1  2004/07/16 13:40:42  tsuruoka
 * init
 *
 */

