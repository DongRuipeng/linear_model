#pragma once
#include <armadillo>
#include <string>

class Linear_Regression
{
public:
	// Linear_Model(arma::mat X, arma::vec y);
	Linear_Regression(arma::mat X, arma::vec y, double lambda = NULL, std::string mode = "coordinate");
	~Linear_Regression();
	void show();
	arma::vec get_estimator();

private:
	// data
	struct Solution_Path
	{
		arma::mat hbeta_path;
		arma::mat C_path;
	};
	Solution_Path path;
	arma::vec hbeta;
	std::string algorithm;
	// function
	arma::vec get_sign(arma::vec x);
	double soft_threshold(double z, double lambda);
	arma::vec extract(Solution_Path path, double lambda);
	Linear_Regression::Solution_Path lars_path(arma::mat X, arma::vec y);
	arma::vec coordinate_descent(arma::mat X, arma::vec y, double lambda);
	arma::vec ols(arma::mat X, arma::vec y);
	arma::vec scaled_lasso(arma::mat X, arma::vec y, double lambda);
};

class SVM
{
public:
	SVM(arma::mat X, arma::vec y, std::string mode = "linear", double C = INFINITY);
	~SVM();
	void show();

private:
	// data
	arma::vec alpha, w;
	arma::mat support_vector;
	arma::vec support_label;
	std::string kernel_type;
	double b;
	// function
	arma::vec solver(arma::mat Q, arma::vec y, double C);
	arma::mat get_matrix_Q(arma::mat X, arma::vec y);
	double kernel(arma::Row<double> a, arma::Row<double> b, std::string mode = "linear");
};
