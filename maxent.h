/*
 * $Id: maxent.h,v 1.7 2004/07/29 05:51:13 tsuruoka Exp $
 */

#ifndef __MAXENT_H_
#define __MAXENT_H_

#include <string>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <iostream>
#include <string>
#include <cassert>
//#include "blmvm.h"

// the data format of samples for training and testing
struct ME_Sample
{
  std::string label;
  std::list<std::string> features;
};

// for those who want to use load_from_array()
typedef struct ME_Model_Data
{
  char * label;
  char * feature;
  double weight;
} ME_Model_Data;

class ME_Model
{
public:

  int train(const std::vector<ME_Sample> & train,
            const int cutoff = 0, const double sigma = 0, const double widthfactor = 0);
  std::vector<double> classify(ME_Sample & s) const;
  bool load_from_file(const std::string & filename);
  bool save_to_file(const std::string & filename) const;
  int num_classes() const { return _num_classes; }
  std::string get_class_label(int i) const { return _label_bag.Str(i); }
  void get_features(std::list< std::pair< std::pair<std::string, std::string>, double> > & fl);
  void setparam_heldout(const int h, const int n) { _nheldout = h; _early_stopping_n = n; };
  bool load_from_array(const ME_Model_Data data[]);

  ME_Model() {
    _nheldout = 0;
    _early_stopping_n = 0;
  }

private:  
  
  struct Sample {
    int label;
    std::list<int> positive_features;
  };

  struct ME_Feature
  {
    bool operator<(const ME_Feature & x) const {
      return _body < x._body;
    }
    ME_Feature(const int l, const int f) : _body((l << 24) + f) {
      assert(l >= 0 && l < 256);
      assert(f >= 0 && f <= 0xffffff);
    };
    int label() const { return _body >> 24; }
    int feature() const { return _body & 0xffffff; }
  private:
    unsigned int _body;
  };

  struct ME_FeatureBag
  {
    std::map<ME_Feature, int> mef2id;
    std::vector<ME_Feature> id2mef;
    int Put(const ME_Feature & i) {
      std::map<ME_Feature, int>::const_iterator j = mef2id.find(i);
      if (j == mef2id.end()) {
        int id = id2mef.size();
        id2mef.push_back(i);
        mef2id[i] = id;
        return id;
      }
      return j->second;
    }
    int Id(const ME_Feature & i) const {
      std::map<ME_Feature, int>::const_iterator j = mef2id.find(i);
      if (j == mef2id.end()) {
        return -1;
      }
      return j->second;
    }
    ME_Feature Feature(int id) const {
      assert(id >= 0 && id < (int)id2mef.size());
      return id2mef[id];
    }
    int Size() const {
      return id2mef.size();
    }
  };

  struct StringBag
  {
    std::map<std::string, int> str2id;
    std::vector<std::string> id2str;
    int Put(const std::string & i) {
      std::map<std::string, int>::const_iterator j = str2id.find(i);
      if (j == str2id.end()) {
        int id = id2str.size();
        id2str.push_back(i);
        str2id[i] = id;
        return id;
      }
      return j->second;
    }
    int Id(const std::string & i) const {
      std::map<std::string, int>::const_iterator j = str2id.find(i);
      if (j == str2id.end()) {
        return -1;
      }
      return j->second;
    }
    std::string Str(const int id) const {
      assert(id >= 0 && id < (int)id2str.size());
      return id2str[id];
    }
    int Size() const {
      return id2str.size();
    }
  };

  
  StringBag _label_bag;
  StringBag _featurename_bag;
  double _sigma; // Gaussian prior
  double _inequality_width;
  std::vector<double> _vl;  // vector of lambda
  std::vector<double> _va;  // vector of alpha (for inequality ME)
  std::vector<double> _vb;  // vector of beta  (for inequality ME)
  ME_FeatureBag _fb;
  int _num_classes;
  std::vector<double> _vee;  // empirical expectation
  std::vector<double> _vme;  // empirical expectation
  std::vector< std::vector< std::vector<int> > > _sample2feature;
  std::vector< Sample > _train;
  std::vector< Sample > _heldout;
  double _train_error;   // current error rate on the training data
  double _heldout_error; // current error rate on the heldout data
  int _nheldout;
  int _early_stopping_n;
  std::vector<double> _vhlogl;

  double heldout_likelihood();
  void conditional_probability(const Sample & nbs, std::vector<double> & membp) const;
  int make_feature_bag(const int cutoff);
  int classify(const Sample & nbs, std::vector<double> & membp) const;
  double update_model_expectation();
  int perform_LMVM();
  int perform_GIS(int C);

  // BLMVM
  //  int BLMVMComputeFunctionGradient(BLMVM blmvm, BLMVMVec X,double *f,BLMVMVec G);
  //  int BLMVMComputeBounds(BLMVM blmvm, BLMVMVec XL, BLMVMVec XU);
  int BLMVMSolve(double *x, int n);
  int BLMVMFunctionGradient(double *x, double *f, double *g, int n);
  int BLMVMLowerAndUpperBounds(double *xl,double *xu,int n);
  //  int Solve_BLMVM(BLMVM blmvm, BLMVMVec X);
  
};



#endif


/*
 * $Log: maxent.h,v $
 * Revision 1.7  2004/07/29 05:51:13  tsuruoka
 * remove modeldata.h
 *
 * Revision 1.6  2004/07/28 13:42:58  tsuruoka
 * add AGIS
 *
 * Revision 1.5  2004/07/28 05:54:14  tsuruoka
 * get_class_name() -> get_class_label()
 * ME_Feature: bugfix
 *
 * Revision 1.4  2004/07/27 16:58:47  tsuruoka
 * modify the interface of classify()
 *
 * Revision 1.3  2004/07/26 17:23:46  tsuruoka
 * _sample2feature: list -> vector
 *
 * Revision 1.2  2004/07/26 15:49:23  tsuruoka
 * modify ME_Feature
 *
 * Revision 1.1  2004/07/26 13:10:55  tsuruoka
 * add files
 *
 * Revision 1.18  2004/07/22 08:34:45  tsuruoka
 * modify _sample2feature[]
 *
 * Revision 1.17  2004/07/21 16:33:01  tsuruoka
 * remove some comments
 *
 */
