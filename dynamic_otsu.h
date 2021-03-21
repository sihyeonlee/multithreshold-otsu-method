#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include <algorithm>
#include <limits>

using namespace std;

vector<int> get_histogram(cv::Mat origin);
vector<double> normalize_histogram(vector<int> histogram, int M, int L);
vector<int> find_valleys(vector<double> norm_hist);
vector<int> threshold_valley_regions(vector<int> histogram, vector<int> valleys, int N);
int otsu_method(vector<int> histogram);

/*
	vector<int> get_histogram(cv::Mat origin)

	Parameters
	---
	origin: cv::Mat (256 Gray-scale Image)

	Return
	---
	'origin' Image histogram: vector<int>

	Info
	---
	Return image histogram type to vector<int>.
*/
vector<int> get_histogram(cv::Mat origin) 
{
	vector<int> histogram(256);

	for (int i = 0; i < origin.rows; i++) {
		for (int j = 0; j < origin.cols; j++)
		{
			int val = origin.at<uchar>(i, j);
			histogram[val]++;
		}
	}

	return histogram;
}

/*
	vector<double> normalize_histogram(vector<int> histogram, int M, int L)

	Parameters
	---
	histogram: vector<int>			See: vector<int> get_histogram(cv::Mat origin)
	M: int							Divided Num for Sets [EX: 256 Gray level image divded 'M' groups -> One Group has N pixels 'N = 256 / M']
	L: int							Image Gray Level Scale [EX: 256]

	Return
	---
	normalized histogram binning: vector<double>

	Info
	---
	Return normalize histogram type to vector<double>.
	Normalized histogram binning reduces noise since bin grouping operates like a smoothing spatial filter.
	Bin grouping also greatly increases the stability of the valley estimation process.

	Page 6(5636). AUTOMATIC MULTILEVEL THRESHOLDING WITH CLUSTER DETERMINATION, ICIC International, 2011
*/
vector<double> normalize_histogram(vector<int> histogram, int M, int L)
{
	vector<double> norm_hist(M);
	int N = L / M;
	double max;

	for (int i = 0; i < L; i++) {
		norm_hist[i / N] += histogram[i];
	}

	max = *max_element(norm_hist.begin(), norm_hist.end());

	for (int i = 0; i < norm_hist.size(); i++) {
		//norm_hist[i] = round(norm_hist[i] / max * 100.0);
		norm_hist[i] = norm_hist[i] / max * 100.0;
	}

	return norm_hist;
}

/*
	vector<int> find_valleys(vector<double> norm_hist)

	Parameters
	---
	norm_hist: vector<double>			See: vector<double> normalize_histogram(vector<int> histogram, int M, int L)

	Return
	---
	valleys: vector<int>

	Info
	---
	Return valleys type to vector<int>.
	The probabilities for the 9 combinations of bin Group Ci can be given as follows,
	Pr(Ci) = 0%			if (H(i) > H(i-1))		||		(H(i) > H(i+1))
	Pr(Ci) = 25%		if (H(i) < H(i-1))		&&		(H(i) == H(i+1))
	Pr(Ci) = 75%		if (H(i) == H(i-1))		&&		(H(i) < H(i+1))
	Pr(Ci) = 100%		if (H(i) < H(i-1))		&&		(H(i) < H(i+1))
	Pr(Ci) = Pr(Ci-1)	if (H(i) == H(i-1))		&&		(H(i) == H(i+1))
	Pr(C0) = Pr(CM-1) = 0%		M - norm_hist.size()

	The probabilities determind in the prvious stage are then added together only if the probability of the scanning counter Ci is not equal to zero.
	If the sum of probabilities is higher than or equal to 100%, the probability of Ci is reset to 100%. Otherwise, it is 0%.
	Pr(Ci) = 0%			if (Pr(Ci-1) + Pr(Ci) + Pr(Ci+1)) < 100%
	Pr(Ci) = 100%		if (Pr(Ci-1) + Pr(Ci) + Pr(Ci+1)) >= 100%
	Pr(C0) = Pr(CM-1) = 0%		M - norm_hist.size()

	Page 6 ~ 7(5636 ~ 5637). AUTOMATIC MULTILEVEL THRESHOLDING WITH CLUSTER DETERMINATION, ICIC International, 2011
*/
vector<int> find_valleys(vector<double> norm_hist)
{
	vector<int> valleys;
	vector<double> probs(norm_hist.size());

	for (int i = 1; i < norm_hist.size() - 1; i++) {
		if (norm_hist[i] > norm_hist[i - 1] || norm_hist[i] > norm_hist[i + 1]) probs[i] = 0.0;
		else if (norm_hist[i] < norm_hist[i - 1] && norm_hist[i] == norm_hist[i + 1]) probs[i] = 0.25;
		else if (norm_hist[i] == norm_hist[i - 1] && norm_hist[i] < norm_hist[i + 1]) probs[i] = 0.75;
		else if (norm_hist[i] < norm_hist[i - 1] && norm_hist[i] < norm_hist[i + 1]) probs[i] = 1.0;
		else if (norm_hist[i] == norm_hist[i - 1] && norm_hist[i] == norm_hist[i + 1]) probs[i] = probs[i - 1];
	}

	vector<int> crobs(probs.size());

	for (int i = probs.size()-2; i > 0; i--)
	{
		if (probs[i] != 0) {
			if (probs[i - 1] + probs[i] + probs[i + 1] >= 1.0) crobs[i] = 1;
			else crobs[i] = 0;
		}
		else crobs[i] = 0;
	}

	//for (int i = 1; i < probs.size() - 1; i++)
	//{
	//	if (probs[i - 1] + probs[i] + probs[i + 1] >= 1.0 && probs[i] != 0) crobs[i] = 1;
	//	else crobs[i] = 0;
	//}

	for (int i = 0; i < crobs.size(); i++) {
		if (crobs[i] > 0) valleys.push_back(i);
	}

	return valleys;
}

/*
	vector<int> threshold_valley_regions(vector<int> histogram, vectr<int> valleys, int N)

	Parameters
	---
	histogram: vector<int>			See: vector<int> get_histogram(cv::Mat origin)
	valleys: vector<int>			See: vector<int> find_valleys(vector<double> norm_hist)
	N: int							Number of pixels in a group ['N = 256 / M']

	Return
	---
	thresholds: vector<int>

	Info
	---
	Return thresholds type to vector<int>.
	Implementation OTSU Method about each valley histogram.
	TSMO method ar always close to the one decided by bilevel Otsu's thresholding.

	Page 8(5638). AUTOMATIC MULTILEVEL THRESHOLDING WITH CLUSTER DETERMINATION, ICIC International, 2011
*/
vector<int> threshold_valley_regions(vector<int> histogram, vector<int> valleys, int N) {
	vector<int> thresholds;

	for (int i = 0; i < valleys.size(); i++) {
		vector<int> temp_hist;
		int start_pos = (valleys[i] * N) - N;
		int end_pos = (valleys[i] + 2) * N;

		for (int idx = start_pos; idx < end_pos; idx++) {
			temp_hist.push_back(histogram[idx]);
		}

		thresholds.push_back(otsu_method(temp_hist) + start_pos);
	}

	return thresholds;
}

/*
	int otsu_method(vector<int> histogram)

	Parameters
	---
	histogram: vector<int>			See: vector<int> get_histogram(cv::Mat origin)

	Return
	---
	threshold: int

	Info
	---
	Return threshold type to int.
	Implementation OTSU Method about input histogram.
	Bilevel thresholding, Otsu's method is a very popular nonparametric thresholding technique,
	which searches for an optimum threshold by maximizing the between-class variance in a gray image.

	Page 3 ~ 4(5633 ~ 5634). AUTOMATIC MULTILEVEL THRESHOLDING WITH CLUSTER DETERMINATION, ICIC International, 2011
*/
int otsu_method(vector<int> histogram) {
	int num_bins = histogram.size();
	int total = 0;
	int sum_total = 0;

	for (int i = 0; i < num_bins; i++) {
		total += histogram[i];
		sum_total += i * histogram[i];
	}

	double weight_background = 0.0;
	double sum_background = 0.0;

	int optimum_value = 0;
	double maximum = DBL_MIN;

	for (int i = 0; i < num_bins; i++) {
		weight_background += histogram[i];
		
		if (weight_background == 0) continue;
		
		int weight_foreground = total - weight_background;

		if (weight_foreground == 0) break;

		sum_background += i * (double)histogram[i];
		double mean_foreground = (sum_total - sum_background) / weight_foreground;
		double mean_background = sum_background / weight_background;

		double inter_class_variance = weight_background * weight_foreground * pow(mean_background - mean_foreground, 2);

		if (inter_class_variance > maximum) {
			optimum_value = i;
			maximum = inter_class_variance;
		}
	}

	return optimum_value;
}