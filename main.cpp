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

string bidir_postag(const string & s, const vector<ME_Model> & vme, const vector<ME_Model> & cvme, bool dont_tokenize);
void bidir_chunking(vector<Sentence> & vs, const vector<ME_Model> & vme);
void init_morphdic();

void help()
{
  cout << "Usage: geniatagger [OPTION]... [FILE]..." << endl;
  cout << "Analyze English sentences and print the base forms, part-of-speech tags, and" << endl;
  cout << "chunk tags." << endl;
  cout << endl;
  cout << "Options:" << endl;
  cout << "  -nt        don't perform tokenization" << endl;
  cout << "  --help     display this help and exit" << endl;
  cout << endl;
  cout << "Report bugs to <tsuruoka@is.s.u-tokyo.ac.jp>." << endl;
}

void version()
{
  cout << "GENIA Tagger 2.0.1" << endl << endl;
}

int main(int argc, char** argv)
{
  bool dont_tokenize = false;
  
  istream *is(&std::cin);

  string ifilename, ofilename;
  for (int i = 1; i < argc; i++) {
    string v = argv[i];
    if (v == "-nt") { dont_tokenize = true; continue; }
    if (v == "--help") { help(); exit(0); }
    ifilename = argv[i];
  }
  ifstream ifile;
  if (ifilename != "" && ifilename != "-") {
    ifile.open(ifilename.c_str());
    if (!ifile) { cerr << "error: cannot open " << ifilename << endl; exit(1); }
    is = &ifile;
  }
                                                                      
  init_morphdic();

  vector<ME_Model> vme(16);

  cerr << "loading pos_models";
  for (int i = 0; i < 16; i++) {
    char buf[1000];
    sprintf(buf, "./models_medline/model.bidir.%d", i);
    vme[i].load_from_file(buf);
    cerr << ".";
  }
  cerr << "done." << endl;

  cerr << "loading chunk_models";
  vector<ME_Model> vme_chunking(16);
  for (int i = 0; i < 8; i +=2 ) {
    char buf[1000];
    sprintf(buf, "./models_chunking/model.bidir.%d", i);
    vme_chunking[i].load_from_file(buf);
    cerr << ".";
  }
  cerr << "done." << endl;
  
  string line;
  while (getline(*is, line)) {
    string postagged = bidir_postag(line, vme, vme_chunking, dont_tokenize);
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

