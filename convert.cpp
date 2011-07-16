#include <string>
#include "common.h"
#include <iostream>

using namespace std;

static void convert_iob_to_startend_sub(Sentence & s)
{
  for (int i = 0; i < s.size(); i++) {
    string tag = s[i].tag;
    //    cout << tag << "\t";
    string nexttag = "O";
    string newtag = tag;
    if (i < s.size() - 1) nexttag = s[i+1].tag;
    if (tag[0] == 'I' && (i == 0 || s[i-1].tag[0] == 'O')) {
      tag = "B" + tag.substr(1);
      newtag = tag;
    }
    if (tag[0] == 'B' && nexttag[0] != 'I') {
      newtag = "S" + tag.substr(1);
    }
    if (tag[0] == 'I') {
      if (nexttag[0] == 'B' || nexttag[0] == 'O') {
        newtag = "E" + tag.substr(1);
      }
    }
    s[i].tag = newtag;
    //    cout << newtag << endl;
  }
  //  cout << endl;
}

static void convert_startend_to_iob2_sub(vector<string> & s)
{
  for (int i = 0; i < s.size(); i++) {
    string tag = s[i];
    string newtag = tag;
    if (tag[0] == 'S') {
      newtag = "B" + tag.substr(1);
    }
    if (tag[0] == 'E') {
      newtag = "I" + tag.substr(1);
    }
    s[i] = newtag;
  }
}

static void print_sentence(const Sentence & s)
{
  for (int i = 0; i < s.size(); i++) {
    cout << s[i].tag << " ";
  }
  cout << endl;
}

void convert_prd_startend_to_iob2(vector<Sentence> & vs)
{
  for (vector<Sentence>::iterator i = vs.begin(); i != vs.end(); i++) {
    Sentence & s = *i;
    vector<string> vs;
    for (int j = 0; j < s.size(); j++) vs.push_back(s[j].prd);
    convert_startend_to_iob2_sub(vs);
    for (int j = 0; j < s.size(); j++) s[j].prd = vs[j];
  }
}

void convert_startend_to_iob2(vector<Sentence> & vs)
{
  for (vector<Sentence>::iterator i = vs.begin(); i != vs.end(); i++) {
    Sentence & s = *i;
    vector<string> vs;
    for (int j = 0; j < s.size(); j++) vs.push_back(s[j].tag);
    convert_startend_to_iob2_sub(vs);
    for (int j = 0; j < s.size(); j++) s[j].tag = vs[j];
  }
}

  
void convert_iob_to_startend(vector<Sentence> & vs)
{
  for (vector<Sentence>::iterator i = vs.begin(); i != vs.end(); i++) {
    Sentence tmps = *i;
    //    print_sentence(*i);
    convert_iob_to_startend_sub(*i);
    Sentence tmps2 = *i;
    //    print_sentence(*i);
    //    convert_startend_to_iob2_sub(tmps2);
    //    print_sentence(*i);
    //    cout << endl;
    /*
    for (int j = 0; j < tmps.size(); j++) {
      if (tmps[j].tag != tmps2[j].tag) {
        cerr << "conversion error" << endl;
        exit(1);
      }
    }
    */
  }
}

