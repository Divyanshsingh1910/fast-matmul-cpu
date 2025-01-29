void init_rand(float* mat, const int m, const int n);

void init_const(float* mat, const float value, const int m, const int n);

struct cmp_result {
    int n_false;
    int n_nans;
    int n_inf;
    int tot_diff;
};

struct cmp_result compare_mats(float* mat, float* mat_ref, const int m, const int n);
void print_mat(float* mat, int m, int n);
