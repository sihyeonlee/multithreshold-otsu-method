#include <opencv2/imgcodecs.hpp>
#include <opencv2/gapi/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "dynamic_otsu.h"

int main(void) {
	std::string path = "sample//";
	cv::Mat input_img;
	int M = 32;
	int L = 256;
	int N = L / M;

	for (const auto& entry : std::filesystem::directory_iterator(path))
	{
		input_img = cv::imread(entry.path().string(), cv::IMREAD_GRAYSCALE);
		vector<int> histogram = get_histogram(input_img);
		vector<double> norm_hist = normalize_histogram(histogram, M, L);
		vector<int> valleys = find_valleys(norm_hist);
		vector<int> thresholds = threshold_valley_regions(histogram, valleys, N);

		cout << entry.path() << endl;

		for (int i = 0; i < thresholds.size(); i++) {
			cout << "Threshold[" << i << "] - " << thresholds[i] << endl;
		}

		cout << endl;

	}

	return 0;
}