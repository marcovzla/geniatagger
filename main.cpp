/*
 * $Id: main.cpp,v 1.8 2005/02/16 10:45:39 tsuruoka Exp $
 */

#include <stdio.h>
#include <fstream>
#include <map>
#include <list>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include "maxent.h"
#include "common.h"

using namespace std;

string bidir_postag(const string & s, const vector<ME_Model> & vme, const vector<ME_Model> & cvme, bool dont_tokenize, bool no_ner);
void bidir_chunking(vector<Sentence> & vs, const vector<ME_Model> & vme);
void init_morphdic(bool quiet);

void help()
{
  cout << "Usage: geniatagger [OPTION]... [FILE]..." << endl;
  cout << "Analyze English sentences and print the base forms, part-of-speech tags, " << endl;
  cout << "chunk tags, and named entity tags." << endl;
  cout << endl;
  cout << "Options:" << endl;
  cout << "  -nt                  don't perform tokenization." << endl;
  cout << "  -f --file-paths      interpret input on stdin as paths to files to be parsed." << endl;
  cout << "  -l --no-line-warn    do not warn on long lines." << endl;
  cout << "  -n --no-ner          do not perform any NER tagging (saves time)." << endl;
  cout << "  -o --output-to-file  use ${INPUT_PATH}.gtag as output instead of stdout, use with -f." << endl;
  cout << "  -q --quiet           do not print status messages to stderr." << endl;
  cout << "  -h --help            display this help and exit." << endl;
  cout << endl;
  cout << "Report bugs to <tsuruoka@is.s.u-tokyo.ac.jp>." << endl;
}

void version()
{
  cout << "GENIA Tagger 3.0.1" << endl << endl;
}

extern void load_ne_models(bool quiet);

int main(int argc, char** argv)
{
  bool dont_tokenize = false;
  bool quiet = false;
  bool file_paths = false;
  bool output_to_file = false;
  bool no_ner = false;
  bool no_line_warn = false;
  string output_file_suffix = "gtag";
  
  istream *is(&std::cin);

  string ifilename = "";

  /* We really should parse the args using getopt, but
   * this will have to do for now even though it leads to some oddities */
  for (int i = 1; i < argc; i++) {
    string v = argv[i];
    if (v == "-nt") { dont_tokenize = true; }
    else if (v == "--help" || v == "-h") { help(); exit(0); }
    else if (v == "--file-paths" || v == "-f") { file_paths = true; }
    else if (v == "--output-to-file" || v == "-o") { output_to_file = true; }
    else if (v == "--no-ner" || v == "-n") { no_ner = true; }
    else if (v == "--no-line-warn" || v == "-l") { no_line_warn = true; }
    else if (v == "--quiet" || v == "-q") { quiet = true; }
    else {
      if (ifilename == "") {
        ifilename = argv[i];
      }
      else {
          /* We only accept one filename argument */
          cerr << "error: more than one input file argument" << endl;
          return -1;
      }
    }
  }
  /* --output-to-file requires --file-paths or it will have no effect, verify */
  if (!file_paths && output_to_file) {
    cerr << "error: -o [--output-to-file] only works with -f [--file-paths]"
        << endl;
    return -1;
  }
  ifstream ifile;
  if (ifilename != "" && ifilename != "-") {
    ifile.open(ifilename.c_str());
    if (!ifile) { cerr << "error: cannot open " << ifilename << endl; exit(1); }
    is = &ifile;
  }
                                                                      
  init_morphdic(quiet);

  vector<ME_Model> vme(16);

  if (!quiet) {
    cerr << "loading pos_models";
  }
  for (int i = 0; i < 16; i++) {
    char buf[1000];
    sprintf(buf, "./models_medline/model.bidir.%d", i);
    vme[i].load_from_file(buf);
    if (!quiet) {
      cerr << ".";
    }
  }
  if (!quiet) {
    cerr << "done." << endl;
  }

  if (!quiet) {
    cerr << "loading chunk_models";
  }
  vector<ME_Model> vme_chunking(16);
  for (int i = 0; i < 8; i +=2 ) {
    char buf[1000];
    sprintf(buf, "./models_chunking/model.bidir.%d", i);
    vme_chunking[i].load_from_file(buf);
    if (!quiet) {
      cerr << ".";
    }
  }
  if (!quiet) {
    cerr << "done." << endl;
  }

  if (!no_ner) {
    load_ne_models(quiet);
  }
 
  if (!file_paths) {
    string line;
    int n = 1;
    while (getline(*is, line)) {
      if (!no_line_warn && line.size() > 1024) {
        cerr << "warning: the sentence seems to be too long at line " << n;
        cerr << " (please note that the input should be one-sentence-per-line)." << endl;
      }
      string postagged = bidir_postag(line, vme, vme_chunking, dont_tokenize, no_ner);
      cout << postagged << endl;
      n++;
    }
  }
  else {
    /* Interpret stdin as a list of file paths then parse them */
    string input_file_path;
    while (getline(*is, input_file_path)) {
      if (input_file_path == "\n") { /* TODO: Does this even work? */
        /* Ignore blank lines */
        continue;
      }

      ifstream input_file(input_file_path.c_str());
      string line;
      int line_number;
      string postagged;
      string output_file_path;

      /* Determine if we are to use an output file or stdout */
      if (!output_to_file) {
        output_file_path = "/dev/stdout";
      }
      else {
        output_file_path = input_file_path + "." + output_file_suffix;
        /* Better safe than sorry, exit if the target output file exists */
        /* TODO: Even better with an overwrite flag */
        struct stat dummyStat; 
        if (stat(output_file_path.c_str(), &dummyStat) == 0) {
          cerr << "error: file " << output_file_path << " already exists" << endl;
          return -1;
        }
      }
      ofstream output_file(output_file_path.c_str());

      line_number = 1;
      while (getline(input_file, line)) {
        if (!no_line_warn && line.size() > 1024) {
          cerr << "warning: line " << line_number << " in file " <<
              input_file_path << " is longer than 1024 characters, "
              "please note that input files "
              "are to consist of one sentence per line." << endl;
          cerr << "    the violating line was \"" << line << "\"" << endl;
        }

        postagged = bidir_postag(line, vme, vme_chunking, dont_tokenize, no_ner);
        output_file << postagged << endl;
        line_number++;
      }
    }
  }
}
