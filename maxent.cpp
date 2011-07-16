/*
 * $Id: maxent.cpp,v 1.7 2004/07/28 13:42:58 tsuruoka Exp $
 */

#include "maxent.h"
#include <cmath>
#include <cstdio>

using namespace std;

int
ME_Model::BLMVMFunctionGradient(double *x, double *f, double *g, int n)
{
  const int nf = _fb.Size();

  if (_inequality_width > 0) {
    assert(nf == n/2);
    for (int i = 0; i < nf; i++) {
      _va[i] = x[i];
      _vb[i] = x[i + nf];
      _vl[i] = _va[i] - _vb[i];
    }
  } else {
    assert(nf == n);
    for (int i = 0; i < n; i++) {
      _vl[i] = x[i];
    }
  }
  
  double score = update_model_expectation();

  if (_inequality_width > 0) {
    for (int i = 0; i < nf; i++) {
      g[i]      = -(_vee[i] - _vme[i] - _inequality_width);
      g[i + nf] = -(_vme[i] - _vee[i] - _inequality_width);
    }
  } else {
    if (_sigma == 0) {
      for (int i = 0; i < n; i++) {
        g[i] = -(_vee[i] - _vme[i]);
      }
    } else {
      const double c = 1 / (_sigma * _sigma);
      for (int i = 0; i < n; i++) {
        g[i] = -(_vee[i] - _vme[i] - c * _vl[i]);
      }
    }
  }

  *f = -score;

  return 0;
}

int
ME_Model::BLMVMLowerAndUpperBounds(double *xl,double *xu,int n)
{
  if (_inequality_width > 0) {
    for (int i = 0; i < n; i++){
      xl[i] = 0;
      xu[i] = 10000.0;
    }
    return 0;
  }

  for (int i = 0; i < n; i++){
    xl[i] = -10000.0;
    xu[i] = 10000.0;
  }
  return 0;
}

int
ME_Model::perform_GIS(int C)
{
  cerr << "C = " << C << endl;
  C = 1;
  cerr << "performing AGIS" << endl;
  vector<double> pre_v;
  double pre_logl = -999999;
  for (int iter = 0; iter < 200; iter++) {

    double logl =  update_model_expectation();
    fprintf(stderr, "iter = %2d  C = %d  f = %10.7f  train_err = %7.5f", iter, C, logl, _train_error);
    if (_heldout.size() > 0) {
      double hlogl = heldout_likelihood();
      fprintf(stderr, "  heldout_logl(err) = %f (%6.4f)", hlogl, _heldout_error);
    }
    cerr << endl;

    if (logl < pre_logl) {
      C += 1;
      _vl = pre_v;
      iter--;
      continue;
    }
    if (C > 1 && iter % 10 == 0) C--;

    pre_logl = logl;
    pre_v = _vl;
    for (int i = 0; i < _fb.Size(); i++) {
      double coef = _vee[i] / _vme[i];
      _vl[i] += log(coef) / C;
    }
  }
  cerr << endl;

}

int
ME_Model::perform_LMVM()
{
  cerr << "performing LMVM" << endl;

  if (_inequality_width > 0) {
    int nvars = _fb.Size() * 2;
    double *x = (double*)malloc(nvars*sizeof(double)); 

    // INITIAL POINT
    for (int i = 0; i < nvars / 2; i++) {
      x[i] = _va[i];
      x[i + _fb.Size()] = _vb[i];
    }

    //    int info = BLMVMSolve(x, nvars);

    for (int i = 0; i < nvars / 2; i++) {
      _va[i] = x[i];
      _vb[i] = x[i + _fb.Size()];
      _vl[i] = _va[i] - _vb[i];
    }
  
    free(x);

    return 0;
  }

  int nvars = _fb.Size();
  double *x = (double*)malloc(nvars*sizeof(double)); 

  // INITIAL POINT
  for (int i = 0; i < nvars; i++) { x[i] = _vl[i]; }

  //  int info = BLMVMSolve(x, nvars);

  for (int i = 0; i < nvars; i++) { _vl[i] = x[i]; }
  
  free(x);

  return 0;
}

void
ME_Model::conditional_probability(const Sample & nbs,
                                  std::vector<double> & membp) const
{
  int num_classes = membp.size();
  double sum = 0;
  for (int label = 0; label < num_classes; label++) {
    double pow = 0.0;
    for (std::list<int>::const_iterator i = nbs.positive_features.begin(); i != nbs.positive_features.end(); i++){
      ME_Feature f(label, *i);
      int id = _fb.Id(f);
      if (id >= 0) {
        pow += _vl[id];
      }
    }
    double prod = exp(pow);
    membp[label] = prod;
    sum += prod;
  }
  for (int label = 0; label < num_classes; label++) {
    membp[label] /= sum;
  }
  
}

int
ME_Model::make_feature_bag(const int cutoff)
{
  int max_label = 0;
  int max_num_features = 0;
  for (std::vector<Sample>::const_iterator i = _train.begin(); i != _train.end(); i++) {
    max_label = max(max_label, i->label);
  }
  _num_classes = max_label + 1;

  //  map< int, list<int> > feature2sample;

  // counting for cutoff
  ME_FeatureBag tmpfb;
  map<int, int> count;
  if (cutoff > 0) {
    for (std::vector<Sample>::const_iterator i = _train.begin(); i != _train.end(); i++) {
      for (std::list<int>::const_iterator j = i->positive_features.begin(); j != i->positive_features.end(); j++) {
        int id = tmpfb.Put(ME_Feature(i->label, *j));
        count[id]++;
      }
    }
  }

  int n = 0;
  for (std::vector<Sample>::const_iterator i = _train.begin(); i != _train.end(); i++, n++) {
    max_num_features = max(max_num_features, (int)(i->positive_features.size()));
    for (std::list<int>::const_iterator j = i->positive_features.begin(); j != i->positive_features.end(); j++) {
      if (cutoff > 0) {
        int id = tmpfb.Id(ME_Feature(i->label, *j));
        if (count[id] <= cutoff) continue;
      }
      int id = _fb.Put(ME_Feature(i->label, *j));
      //      cout << i->label << "\t" << *j << "\t" << id << endl;
      //      feature2sample[id].push_back(n);
    }
  }
  count.clear();
  
  //  cerr << "num_classes = " << _num_classes << endl;
  //  cerr << "max_num_features = " << max_num_features << endl;

  int c = 0;
  
  _sample2feature.clear();
  _sample2feature.resize(_train.size());
  /*
  for (size_t i = 0; i < _train.size(); i++) {
    _sample2feature[i].resize(_num_classes);
  }
  for (map<int, list<int> >::const_iterator i = feature2sample.begin(); i != feature2sample.end(); i++) {
    for (list<int>::const_iterator j = i->second.begin(); j != i->second.end(); j++) {
      int l = _fb.Feature(i->first).label;
      _sample2feature[*j][l].push_back(i->first);
      c++;
    }
    
  }
  */

  n = 0;
  for (std::vector<Sample>::const_iterator i = _train.begin(); i != _train.end(); i++) {
    _sample2feature[n].resize(_num_classes);
    for (std::list<int>::const_iterator j = i->positive_features.begin(); j != i->positive_features.end(); j++){
      for (int k = 0; k < _num_classes; k++) {
        int id = _fb.Id(ME_Feature(k, *j));
        if (id >= 0) {
          _sample2feature[n][k].push_back(id);
          c++;
        }
      }
    }
    n++;
  }

  //  cerr << "c = " << c << endl;
  
  return max_num_features;
}

double
ME_Model::heldout_likelihood()
{
  double logl = 0;
  int ncorrect = 0;
  for (std::vector<Sample>::const_iterator i = _heldout.begin(); i != _heldout.end(); i++) {
    vector<double> membp(_num_classes);
    int l = classify(*i, membp);
    logl += log(membp[i->label]);
    if (l == i->label) ncorrect++;
  }
  _heldout_error = 1 - (double)ncorrect / _heldout.size();
  
  return logl /= _heldout.size();
}

double
ME_Model::update_model_expectation()
{
  double logl = 0;
  int ncorrect = 0;
  int n = 0;
  vector< vector<double> > membpv;
  for (std::vector<Sample>::const_iterator i = _train.begin(); i != _train.end(); i++) {
    vector<double> membp(_num_classes);
    int max_label = 0;
    double sum = 0;
    for (int label = 0; label < _num_classes; label++) {
      double pow = 0.0;
      const vector<int> & fl = _sample2feature[n][label];
      for (std::vector<int>::const_iterator i = fl.begin(); i != fl.end(); i++){
        pow += _vl[*i];
      }
      double prod = exp(pow);
      membp[label] = prod;
      sum += prod;
    }
    for (int label = 0; label < _num_classes; label++) {
      membp[label] /= sum;
      if (membp[label] > membp[max_label]) max_label = label;
    }
    
    logl += log(membp[i->label]);
    if (max_label == i->label) ncorrect++;
    membpv.push_back(membp);
    n++;
  }

  // model expectation
  _vme.resize(_fb.Size());
  for (int i = 0; i < _fb.Size(); i++) {
    _vme[i] = 0;
  }
  for (int n = 0; n < (int)_train.size(); n++) {
    for (int i = 0; i < _num_classes; i++) {
      const vector<int> & fl = _sample2feature[n][i];
      for (vector<int>::const_iterator j = fl.begin(); j != fl.end(); j++) {
        _vme[*j] += membpv[n][_fb.Feature(*j).label()];
      }
    }
  }
  for (int i = 0; i < _fb.Size(); i++) {
    _vme[i] /= _train.size();
  }
  
  _train_error = 1 - (double)ncorrect / _train.size();

  logl /= _train.size();
  
  if (_inequality_width > 0) {
    for (int i = 0; i < _fb.Size(); i++) {
      logl -= (_va[i] + _vb[i]) * _inequality_width;
    }
  } else {
    if (_sigma > 0) {
      const double c = 1/(2*_sigma*_sigma);
      for (int i = 0; i < _fb.Size(); i++) {
        logl -= _vl[i] * _vl[i] * c;
      }
    }
  }
  
  //  fprintf(stderr, "iter =%3d  logl = %10.7f  train_acc = %7.5f\n", iter, logl, (double)ncorrect/train.size());
  //  fprintf(stderr, "logl = %10.7f  train_acc = %7.5f\n", logl, (double)ncorrect/_train.size());

  return logl;
}

int
ME_Model::train(const vector<ME_Sample> & vms, const int cutoff,
                const double sigma, const double widthfactor)
{
  if (sigma > 0 && widthfactor > 0) {
    cerr << "warning: Gausian prior and inequality ME cannot be used at the same time." << endl;
  }
  if (_nheldout >= vms.size()) {
    cerr << "error: too much heldout data. no training data is available." << endl;
    return 0;
  }
  
  // convert ME_Sample to Sample
  vector<Sample> vs;
  for (vector<ME_Sample>::const_iterator i = vms.begin(); i != vms.end(); i++) {
    Sample s;
    s.label = _label_bag.Put(i->label);
    for (list<string>::const_iterator j = i->features.begin(); j != i->features.end(); j++) {
      s.positive_features.push_back(_featurename_bag.Put(*j));
    }
    vs.push_back(s);
  }
  
  for (int i = 0; i < (int)vs.size() - _nheldout; i++) _train.push_back(vs[i]);
  for (int i = vs.size() - _nheldout; i < (int)vs.size(); i++) _heldout.push_back(vs[i]);
  vs.clear();
  
  _sigma = sigma;
  _inequality_width = widthfactor / _train.size();
  
  if (cutoff > 0)
    cerr << "cutoff threshold = " << cutoff << endl;
  if (sigma > 0)
    cerr << "Gaussian prior sigma = " << sigma << endl;
  if (widthfactor > 0)
    cerr << "widthfactor = " << widthfactor << endl;
  cerr << "making feature bag...";
  int C = make_feature_bag(cutoff);
  cerr << "done" << endl;
  cerr << "number of samples = " << _train.size() << endl;
  cerr << "number of features = " << _fb.Size() << endl;

  cerr << "calculating empirical expectation...";
  _vee.resize(_fb.Size());
  for (int i = 0; i < _fb.Size(); i++) {
    _vee[i] = 0;
  }
  for (int n = 0; n < (int)_train.size(); n++) {
    const vector<int> & fl = _sample2feature[n][_train[n].label];
    for (vector<int>::const_iterator j = fl.begin(); j != fl.end(); j++) {
      //      if (_fb.Feature(*j).label == _train[n].label)
      assert(_fb.Feature(*j).label() == _train[n].label);
      _vee[*j] += 1.0;
    }
  }
  for (int i = 0; i < _fb.Size(); i++) {
    _vee[i] /= _train.size();
  }
  cerr << "done" << endl;
  
  _vl.resize(_fb.Size());
  for (int i = 0; i < _fb.Size(); i++) _vl[i] = 0.0;
  if (_inequality_width > 0) {
    _va.resize(_fb.Size());
    _vb.resize(_fb.Size());
    for (int i = 0; i < _fb.Size(); i++) {
      _va[i] = 0.0;
      _vb[i] = 0.0;
    }
  }

  //perform_GIS(C);
  perform_LMVM();

  if (_inequality_width > 0) {
    int sum = 0;
    for (int i = 0; i < _fb.Size(); i++) {
      if (_vl[i] != 0) sum++;
    }
    cerr << "number of active features = " << sum << endl;
  }
  
}

void
ME_Model::get_features(list< pair< pair<string, string>, double> > & fl)
{
  fl.clear();
  for (int i = 0; i < _fb.Size(); i++) {
    ME_Feature f = _fb.Feature(i);
    fl.push_back( make_pair(make_pair(_label_bag.Str(f.label()), _featurename_bag.Str(f.feature())), _vl[i]));
  }
}

bool
ME_Model::load_from_file(const string & filename)
{
  FILE * fp = fopen(filename.c_str(), "r");
  if (!fp) {
    cerr << "error: cannot open " << filename << "!" << endl;
    return false;
  }

  _vl.clear();
  char buf[1024];
  while(fgets(buf, 1024, fp)) {
    char classname[256], featurename[512];
    float lambda;
    sscanf(buf, "%s\t%s\t%f", classname, featurename, &lambda);
    int label = _label_bag.Put(classname);
    int feature = _featurename_bag.Put(featurename);
    _fb.Put(ME_Feature(label, feature));
    _vl.push_back(lambda);
  }
    
  _num_classes = _label_bag.Size();

  fclose(fp);

  return true;
}

bool
ME_Model::load_from_array(const ME_Model_Data data[])
{
  _vl.clear();
  for (int i = 0;; i++) {
    if (string(data[i].label) == "///") break;
    int label = _label_bag.Put(data[i].label);
    int feature = _featurename_bag.Put(data[i].feature);
    _fb.Put(ME_Feature(label, feature));
    _vl.push_back(data[i].weight);
  }
  _num_classes = _label_bag.Size();
  return true;
}

bool
ME_Model::save_to_file(const string & filename) const
{
  FILE * fp = fopen(filename.c_str(), "w");
  if (!fp) {
    cerr << "error: cannot open " << filename << "!" << endl;
    return false;
  }

  for (int i = 0; i < _fb.Size(); i++) {
    if (_vl[i] == 0) continue; // ignore zero-weight features
    ME_Feature f = _fb.Feature(i);
    fprintf(fp, "%s\t%s\t%f\n", _label_bag.Str(f.label()).c_str(), _featurename_bag.Str(f.feature()).c_str(), _vl[i]);
  }

  fclose(fp);

  return true;
}

int
ME_Model::classify(const Sample & nbs, vector<double> & membp) const
{
  //  vector<double> membp(_num_classes);
  assert(_num_classes == (int)membp.size());
  conditional_probability(nbs, membp);
  int max_label = 0;
  double max = 0.0;
  for (int i = 0; i < (int)membp.size(); i++) {
    //    cout << membp[i] << " ";
    if (membp[i] > max) { max_label = i; max = membp[i]; }
  }
  //  cout << endl;
  return max_label;
}

vector<double>
ME_Model::classify(ME_Sample & mes) const
{
  Sample s;
  for (list<string>::const_iterator j = mes.features.begin(); j != mes.features.end(); j++) {
    int id = _featurename_bag.Id(*j);
    if (id >= 0)
      s.positive_features.push_back(id);
  }

  vector<double> vp(_num_classes);
  int label = classify(s, vp);
  mes.label = get_class_label(label);
  return vp;
}

/*
 * $Log: maxent.cpp,v $
 * Revision 1.7  2004/07/28 13:42:58  tsuruoka
 * add AGIS
 *
 * Revision 1.6  2004/07/28 05:54:13  tsuruoka
 * get_class_name() -> get_class_label()
 * ME_Feature: bugfix
 *
 * Revision 1.5  2004/07/27 16:58:47  tsuruoka
 * modify the interface of classify()
 *
 * Revision 1.4  2004/07/26 17:23:46  tsuruoka
 * _sample2feature: list -> vector
 *
 * Revision 1.3  2004/07/26 15:49:23  tsuruoka
 * modify ME_Feature
 *
 * Revision 1.2  2004/07/26 13:52:18  tsuruoka
 * modify cutoff
 *
 * Revision 1.1  2004/07/26 13:10:55  tsuruoka
 * add files
 *
 * Revision 1.20  2004/07/22 08:34:45  tsuruoka
 * modify _sample2feature[]
 *
 * Revision 1.19  2004/07/21 16:33:01  tsuruoka
 * remove some comments
 *
 */

