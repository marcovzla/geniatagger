/*
 * $Id$
 */

#include <iostream>
#include "maxent.h"

extern ME_Model_Data me_model_data[];

using namespace std;

string postag(const string & s, const ME_Model & me);

int main()
{
  ME_Model me;
  //  cerr << "initializing the model...";
  //  me.load_from_file("./model.wsj");
  me.load_from_array(me_model_data);
  //  cerr << "done" << endl;

  string line;
  while (getline(cin, line)) {
    string postagged = postag(line, me);
    cout << postagged << endl;
  }
  
}

/*
 * $Log$
 */

