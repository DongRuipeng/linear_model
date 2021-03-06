#include "Linear_Model.h"

Linear_Regression::Linear_Regression(arma::mat X, arma::vec y, double lambda, std::string mode)
{
	if (lambda == NULL)
	{
		Linear_Regression::hbeta = ols(X, y);
		Linear_Regression::algorithm = "ols";
		return;
	}
	else if (mode == "lars")
	{
		Linear_Regression::path = lars_path(X, y);
		Linear_Regression::hbeta = extract(Linear_Regression::path, lambda);
	}
	else if (mode == "scaled")
	{
		Linear_Regression::hbeta = scaled_lasso(X, y, lambda);
	}
	else
	{
		Linear_Regression::hbeta = coordinate_descent(X, y, lambda);
	}
	Linear_Regression::algorithm = mode;
}

Linear_Regression::~Linear_Regression()
{
	std::cout << "delete the objetion ... \n";
}

void Linear_Regression::show()
{
	if (Linear_Regression::algorithm == "ols")
	{
		std::cout << "the algorithm is ols. \n";
		std::cout << "hbeta : \n" << Linear_Regression::hbeta << std::endl;
	}
	else if (Linear_Regression::algorithm == "lars")
	{
		std::cout << "the algorithm is least angle regression. \n";
		std::cout << "hbeta : \n" << Linear_Regression::hbeta << std::endl;
		std::cout << "the beta path of lars is : \n";
		std::cout << Linear_Regression::path.hbeta_path << std::endl;
		std::cout << "the correlation path is : \n";
		std::cout << Linear_Regression::path.C_path << std::endl;
	}
	else
	{
		std::cout << "the algorithm is coordinate descent method. \n";
		std::cout << "hbeta : \n" << Linear_Regression::hbeta << std::endl;
	}
}

arma::vec Linear_Regression::get_estimator()
{
	return Linear_Regression::hbeta;
}

arma::vec Linear_Regression::get_sign(arma::vec x)
{
	arma::vec s;
	s.zeros(arma::size(x));
	s(arma::find(x >= 0)).fill(1);
	s(arma::find(x < 0)).fill(-1);
	return s;
}

double Linear_Regression::soft_threshold(double z, double lambda)
{
	if (z > lambda)
	{
		z = z - lambda;
	}
	else if (z < -lambda)
	{
		z = z + lambda;
	}
	else
	{
		z = 0;
	}
	return z;
}

arma::vec Linear_Regression::extract(Solution_Path path, double lambda)
{
	if (lambda > path.C_path[0])
	{
		std::cout << "waring : there is no variable being selected ! \n";
		std::cout << "return a null variable, please reset lambda ! \n";
		return arma::vec();
	}
	unsigned index = path.C_path.n_elem;
	for (unsigned i = 0; i < path.C_path.n_elem; i++)
	{
		if (path.C_path[i] <= lambda)
		{
			index = i;
			break;
		}
	}
	if (index == 0)
	{
		return path.hbeta_path.col(0);
	}
	else if (index == path.C_path.n_elem)
	{
		std::cout << "lambda is too little, return a OLS estimator ! \n";
		return path.hbeta_path.col(path.C_path.n_elem - 1);
	}
	double alpha = (path.C_path[index - 1] - lambda) / (path.C_path[index - 1] - path.C_path[index]);
	arma::vec hbeta = path.hbeta_path.col(index - 1) + alpha * (path.hbeta_path.col(index) - path.hbeta_path.col(index - 1));
	return hbeta;
}

Linear_Regression::Solution_Path Linear_Regression::lars_path(arma::mat X, arma::vec y)
{
	unsigned r_x = arma::rank(X);
	double eps = 1e-5;
	// intilize hmu
	arma::vec hmu = arma::zeros(y.n_elem);
	// initilize the correlation between X and y
	arma::vec hc = X.t() * y;
	// initilize the maximum correlation
	arma::vec max_c = arma::zeros(r_x + 1);
	max_c[0] = arma::max(arma::abs(hc));
	// initilize the active set 
	arma::uvec active_set = arma::find(abs(hc) == max_c[0]);
	// initilize the complement of the active set
	arma::uvec active_set_c = arma::find(abs(hc) != max_c[0]);
	// initilize hbeta
	arma::mat hbeta = arma::zeros(X.n_cols, r_x + 1);
	// define the column number of hbeta;
	unsigned col_num_hbeta;

	for (unsigned i = 0; i < r_x; i++)
	{
		double gama;
		col_num_hbeta = i;
		// get the sign of the correlation in the active set
		arma::vec s = get_sign(hc(active_set));
		// make the angle between X_a and y less than pi/2
		arma::mat X_a = X.cols(active_set) * arma::diagmat(s);
		// get the Gram matrix of X_a
		arma::mat G_a = X_a.t() * X_a;
		// get the inverse of G_a
		arma::mat G_a_inv = arma::inv_sympd(G_a);
		// initilize a one vector
		arma::vec one_vec = arma::ones(active_set.n_elem);
		// get the bisection of X_a
		arma::mat A_a_temp = one_vec.t() * G_a_inv * one_vec;
		double A_a = A_a_temp[0, 0];
		A_a = 1 / sqrt(A_a);
		// get the bisection line of X_a
		arma::vec u_a = A_a * X_a * G_a_inv * one_vec;
		// initilize a temp variable
		arma::vec a = X.t() * u_a;
		// get the gama(i) in the i^th step
		arma::vec temp_gama_1 = (max_c[i] - hc(active_set_c)) / (A_a - a(active_set_c));
		temp_gama_1(arma::find(temp_gama_1 <= 0)).fill(INFINITY);
		arma::vec temp_gama_2 = (max_c[i] + hc(active_set_c)) / (A_a + a(active_set_c));
		temp_gama_2(arma::find(temp_gama_2 <= 0)).fill(INFINITY);
		arma::vec temp_gama = arma::zeros(active_set_c.n_rows);
		for (unsigned j = 0; j < active_set_c.n_rows; j++)
		{
			temp_gama[j] = std::min(temp_gama_1[j], temp_gama_2[j]);
		}
		gama = temp_gama.min();
		arma::vec d = arma::diagmat(s) * A_a * G_a_inv * one_vec;
		arma::vec temp_d = -hbeta.submat(active_set, arma::uvec{ i }) / d;
		temp_d(arma::find(temp_d <= 0)).fill(INFINITY);
		if (gama > temp_d.min())
		{
			gama = temp_d.min();
			// update hbeta
			hbeta.submat(active_set, arma::uvec{ i + 1 }) = hbeta.submat(active_set, arma::uvec{ i }) + gama * d;
			// update max_c
			max_c[i + 1] = max_c[i] - gama * A_a;
			if (max_c[i + 1] <= eps)
			{
				break;
			}
			// update hc and hmu
			hmu = hmu + gama * u_a;
			hc = X.t() * (y - hmu);
			// update the active set and the complement
			arma::uvec index = arma::find(temp_d == temp_d.min()), index_c = arma::find(temp_d != temp_d.min());
			active_set_c.insert_rows(0, active_set.elem(index));
			arma::uvec temp = active_set.elem(index_c);
			active_set.reset();
			active_set = temp;
			/*for (unsigned k = 0; k < index.n_elem; k++)
			{
				active_set.shed_row(index[k]);
			}*/
			continue;
		}
		// update hbeta
		hbeta.submat(active_set, arma::uvec{ i + 1 }) = hbeta.submat(active_set, arma::uvec{ i }) + gama *  d;

		// update max_c
		max_c[i + 1] = max_c[i] - gama * A_a;
		if (max_c[i + 1] <= eps)
		{
			break;
		}
		// update the active set and the complement
		arma::uvec new_x_set = arma::find(temp_gama == gama), new_x_set_c = arma::find(temp_gama != gama);
		active_set.insert_rows(0, active_set_c.elem(new_x_set));
		arma::uvec temp = active_set_c.elem(new_x_set_c);
		active_set_c.reset();
		active_set_c = temp;
		/*for (unsigned k = 0; k < new_x_set.n_elem; k++)
		{
			active_set_c.shed_row(new_x_set[k]);
		}*/
		// update hc and hmu
		hmu = hmu + gama * u_a;
		hc = X.t() * (y - hmu);
		if (active_set_c.n_rows == 0 || i == r_x - 2)
		{
			// std::cout << "complement is null ... \n";
			i = i + 1;
			col_num_hbeta = i;
			// get the sign of the correlation in the active set
			arma::vec s = get_sign(hc(active_set));
			// make the angle between X_a and y less than pi/2
			arma::mat X_a = X.cols(active_set) * arma::diagmat(s);
			// get the Gram matrix of X_a
			arma::mat G_a = X_a.t() * X_a;
			// get the inverse of G_a
			arma::mat G_a_inv = arma::inv_sympd(G_a);
			// initilize a one vector
			arma::vec one_vec = arma::ones(active_set.n_elem);
			// get the bisection of X_a
			arma::mat A_a_temp = one_vec.t() * G_a_inv * one_vec;
			double A_a = A_a_temp[0, 0];
			A_a = 1 / sqrt(A_a);
			gama = max_c[i] / A_a;
			// update hbeta
			hbeta.submat(active_set, arma::uvec{ i + 1 }) = hbeta.submat(active_set, arma::uvec{ i }) + gama * A_a * arma::diagmat(s) * G_a_inv * one_vec;
			// update max_c
			arma::mat temp_max_c = X.t() * ( y - X * hbeta.col(i + 1) );
			max_c[i + 1] = temp_max_c[0, 0];
			//std::cout << max_c[i + 1] << std::endl;
			//std::cout << i + 1 << std::endl;
			break;
		}
		// std::cout << max_c << std::endl;
	}
	col_num_hbeta = col_num_hbeta + 1;
	//std::cout << col_num_hbeta << std::endl;
	/*if (active_set_c.is_empty())
	{
		hbeta.shed_col(0);
		max_c.shed_row(0);
	}
	else
	{
		hbeta.shed_col(0);
		hbeta.shed_cols(col_num_hbeta, hbeta.n_cols - 1);
		max_c.shed_row(0);
		max_c.shed_rows(col_num_hbeta, max_c.n_rows - 1);
	}*/
	arma::mat hbeta_path = hbeta.cols(1, col_num_hbeta);
	arma::vec C_path = max_c.subvec(1, col_num_hbeta);
	/*std::cout << max_c << std::endl;
	std::cout << hbeta.col(1) << std::endl;*/

	Linear_Regression::Solution_Path path;
	path.C_path = C_path;
	path.hbeta_path = hbeta_path;
	return path;
}

arma::vec Linear_Regression::coordinate_descent(arma::mat X, arma::vec y, double lambda)
{
	unsigned p = X.n_cols;
	unsigned MAX_ITERATION = 200;
	unsigned NUM_ITERATION;
	double eps = 1e-5;
	// initilize hbeta
	arma::vec hbeta = arma::zeros(p);
	// initilize r
	arma::vec r = y;
	for (unsigned i = 0; i < MAX_ITERATION; i++)
	{
		// update beta_old
		arma::vec hbeta_old = hbeta;
		for (unsigned j = 0; j < p; j++)
		{
			arma::mat temp_z = (X.col(j).t() * r) / (X.col(j).t() * X.col(j)) + hbeta_old[j];
			double z = temp_z[0, 0];
			// update beta_j
			hbeta[j] = soft_threshold(z, lambda);
			// update r
			r = r - (hbeta[j] - hbeta_old[j])*X.col(j);
		}
		NUM_ITERATION = i + 1;
		if (arma::norm(hbeta - hbeta_old) < eps)
		{
			break;
		}
	}
	if (NUM_ITERATION == MAX_ITERATION)
	{
		std::cout << "the number of the iteration is over MAX_ITERATION ! \n";
	}
	return hbeta;
}

arma::vec Linear_Regression::ols(arma::mat X, arma::vec y)
{
	arma::mat G = X.t() * X;
	arma::vec eigval;
	arma::mat eigvec;
	arma::eig_sym(eigval, eigvec, G);
	for (unsigned i = 0; i < eigval.n_elem; i++)
	{
		if (eigval[i] != 0)
		{
			eigval[i] = 1 / eigval[i];
		}
		else
		{
			eigval[i] = 0;
		}
	}
	arma::mat G_inv = eigvec * arma::diagmat(eigval) * eigvec.t();
	arma::vec hbeta = G_inv * X.t() * y;
	return hbeta;
}

arma::vec Linear_Regression::scaled_lasso(arma::mat X, arma::vec y, double lambda0)
{
	// define the eps, initial sigma and lambda0
	double eps = 1e-5, lambda = lambda0;
	double sigma = 0, sigma_old = arma::norm(y - X * ols(X, y)) / sqrt(X.n_rows);
	unsigned MAX_ITERATION = 100, iter_num = 0;
	// define hbeta
	arma::vec hbeta;
	// initilize the path of lasso
	path = lars_path(X, y);
	while (abs(sigma - sigma_old) > eps && iter_num < MAX_ITERATION)
	{
		lambda = sigma_old * lambda;
		hbeta = extract(path, lambda);
		sigma_old = sigma;
		sigma = norm(y - X * hbeta)/sqrt(X.n_rows);
		iter_num = iter_num + 1;
	}
	if (iter_num == MAX_ITERATION)
	{
		std::cout << "warning: over the maximum iteration number ! \n";
	}
	return hbeta;
}
