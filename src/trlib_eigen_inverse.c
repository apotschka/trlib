#include "trlib.h"
#include "trlib_private.h"

trlib_int_t trlib_eigen_inverse(
        trlib_int_t n, trlib_flt_t *diag, trlib_flt_t *offdiag, 
        trlib_flt_t lam_init, trlib_int_t itmax, trlib_flt_t tol_abs,
        trlib_flt_t *ones, trlib_flt_t *diag_fac, trlib_flt_t *offdiag_fac,
        trlib_flt_t *eig, trlib_int_t verbose, trlib_int_t unicode, char *prefix, FILE *fout,
        trlib_int_t *timing, trlib_flt_t *lam_pert, trlib_flt_t *pert, trlib_int_t *iter_inv) {
    // Local variables
    #if TRLIB_MEASURE_TIME
        struct timespec verystart, start, end;
        TRLIB_TIC(verystart)
    #endif
    *iter_inv = 0;                               // iteration counter
    *pert = 0.0;                                 // perturbation factor to update lam until factorization is possible
    trlib_int_t info_fac = 0;                            // status variable for factorization
    trlib_flt_t invnorm = 0.0;                        // 1/norm of eig before normalization
    trlib_flt_t minuslam = - lam_init;                // negative of current estimation of eigenvalue
    trlib_int_t inc = 1; trlib_int_t nm = n-1;

    // obtain factorization of T - lam*I, perturb until possible
    // iter_inv is misused in this loop as flag if we can find a suitable lambda to start with
    *iter_inv = TRLIB_EIR_FAIL_FACTOR;
    while (*pert <= 1.0/TRLIB_EPS) {
        // set diag_fac to diag - lam
        TRLIB_DCOPY(&n, diag, &inc, diag_fac, &inc) // diag_fac <-- diag
        TRLIB_DAXPY(&n, &minuslam, ones, &inc, diag_fac, &inc) // diag_fac <-- diag_fac - lam
        TRLIB_DCOPY(&nm, offdiag, &inc, offdiag_fac, &inc) // offdiag_fac <-- offdiag
        TRLIB_DPTTRF(&n, diag_fac, offdiag_fac, &info_fac); // compute factorization
        if (info_fac == 0) { *iter_inv = 0; break; }
        if (*pert == 0.0) { 
            *pert = TRLIB_EPS_POW_4 * fmax(1.0, -lam_init);
        }
        else { 
            *pert = 10.0*(*pert);
        }
        minuslam = *pert - lam_init;
    }
    *lam_pert = -minuslam;

    if ( *iter_inv == TRLIB_EIR_FAIL_FACTOR ) { TRLIB_PRINTLN_2("Failure on factorizing in inverse correction!") TRLIB_RETURN(TRLIB_EIR_FAIL_FACTOR) }

    TRLIB_DNRM2(invnorm, &n, eig, &inc) invnorm = 1.0/invnorm;
    TRLIB_DSCAL(&n, &invnorm, eig, &inc) // normalize eig
    // perform inverse iteration
    while (1) {
        *iter_inv += 1;

        if ( *iter_inv > itmax ) { TRLIB_RETURN(TRLIB_EIR_ITMAX) }

        // solve (T - lam*I)*eig_new = eig_old
        TRLIB_DPTTRS(&n, &inc, diag_fac, offdiag_fac, eig, &n, &info_fac)
        if( info_fac != 0 ) { TRLIB_PRINTLN_2("Failure on solving inverse correction!") TRLIB_RETURN(TRLIB_EIR_FAIL_LINSOLVE) }

        // normalize eig
        TRLIB_DNRM2(invnorm, &n, eig, &inc) invnorm = 1.0/invnorm;
        TRLIB_DSCAL(&n, &invnorm, eig, &inc)

        // check for convergence
        if (fabs( invnorm - *pert ) <= tol_abs ) { TRLIB_RETURN(TRLIB_EIR_CONV) }
    }
    
    TRLIB_RETURN(TRLIB_EIR_ITMAX)
};

trlib_int_t trlib_eigen_timing_size() {
#if TRLIB_MEASURE_TIME
    return 1 + TRLIB_SIZE_TIMING_LINALG;
#endif
    return 0;
}

