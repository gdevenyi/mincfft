/* ----------------------------- MNI Header -----------------------------------
@NAME       : fft_support.c
@DESCRIPTION: collection of routines to prepare and manipulate complex data
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Fri Nov  5 11:16:54 EST 1993 Louis Collins
@MODIFIED   : $Log: fft_support.c,v $
@MODIFIED   : Revision 1.2  2002-11-21 01:48:18  rotor
@MODIFIED   : * Changed complex_vector to MIvector_dimension
@MODIFIED   : * large reorganisations to the 2D fft calculations
@MODIFIED   :      to the point that they should be correct now.
@MODIFIED   :
@MODIFIED   : Revision 1.1  2002/09/17 23:44:51  rotor
@MODIFIED   : ----------------------------------------------------------------------
@MODIFIED   : Initial entry of mincfft to CVS (to perhaps replace louis's old NR
@MODIFIED   :    version of mincfft
@MODIFIED   :
@MODIFIED   : Committing in .
@MODIFIED   :
@MODIFIED   : Added Files:
@MODIFIED   :  Makefile fft_support.c fft_support.h mincfft.c
@MODIFIED   : ----------------------------------------------------------------------
@MODIFIED   :
 * Revision 1.2  93/11/08  14:11:16  louis
 * working version, with proper scaling
 * 
 * Revision 1.1  93/11/05  14:19:34  louis
 * Initial revision
 * 
---------------------------------------------------------------------------- */

#include <math.h>
#include <float.h>
#include <fftw.h>
#include "fft_support.h"

/* function prototypes */
Status fft_volume_3d(Volume data, int inverse_flg, int centre);
Status fft_volume_2d(Volume data, int inverse_flg, int centre);

extern char *spac_dimorder[];
extern char *freq_dimorder[];
extern int centre_fft;

Status prep_volume(Volume * in_vol, Volume * out_vol)
{
   int      i, j, k;
   Real     value;
   progress_struct progress;
   Real     min, max;

   int      sizes[4];
   Real     starts[4];
   Real     separations[4];
   Real     tmp_dircos[4];
   
   get_volume_sizes(*in_vol, sizes);
   get_volume_starts(*in_vol, starts);
   get_volume_separations(*in_vol, separations);
   get_volume_real_range(*in_vol, &min, &max);
   
   /* setup frequency dimension */
   sizes[3] = 2;
   starts[3] = 0;
   separations[3] = 1;
   
   /* define new out_vol volume  */
   *out_vol = create_volume(4, freq_dimorder, NC_FLOAT, TRUE, 0.0, 0.0);
   set_volume_sizes(*out_vol, sizes);
   set_volume_starts(*out_vol, starts);
   set_volume_separations(*out_vol, separations);
   set_volume_real_range(*out_vol, min, max);
   
   /* copy over the direction cosines for x, y and z */
   for(i=0; i<3; i++){
      get_volume_direction_cosine(*in_vol, i, tmp_dircos);
      set_volume_direction_cosine(*out_vol, i, tmp_dircos);
      }
   
   /* allocate space for out_vol */
   alloc_volume_data(*out_vol);
   
   initialize_progress_report(&progress, FALSE, sizes[0], "Prep Volume");
   for(i = sizes[0]; i--; ){
      for(j = sizes[1]; j--; ){
         for(k = sizes[2]; k--; ){
            
            GET_VALUE_3D(value, *in_vol, i, j, k);
            set_volume_real_value(*out_vol, i, j, k, 0, 0, value); /* real */
            set_volume_real_value(*out_vol, i, j, k, 1, 0, 0.0);   /* imag */
            }
         }
      update_progress_report(&progress, i + 1);
      }
   
   /* be tidy */
   terminate_progress_report(&progress);

   return (OK);
   }


Status proj_volume(Volume * in_vol, Volume * out_vol, int job)
{
   int      i, j, k;
   Real     value, real, imag;
   Real     min, max;

   int      sizes[4];
   Real     starts[4];
   Real     separations[4];
   Real     tmp_dircos[4];

   get_volume_sizes(*in_vol, sizes);
   get_volume_starts(*in_vol, starts);
   get_volume_separations(*in_vol, separations);

   /* define new out_vol volume  */
   *out_vol = create_volume(3, spac_dimorder, NC_FLOAT, TRUE, 0.0, 0.0);
   set_volume_sizes(*out_vol, sizes);
   set_volume_starts(*out_vol, starts);
   set_volume_separations(*out_vol, separations);
   
   /* copy over the direction cosines for x, y and z */
   for(i=0; i<3; i++){
      get_volume_direction_cosine(*in_vol, i, tmp_dircos);
      set_volume_direction_cosine(*out_vol, i, tmp_dircos);
      }

   /* allocate space for out_vol */
   alloc_volume_data(*out_vol);

   min = DBL_MAX;
   max = -DBL_MAX;

   /* setup the required volume */
   for(i = sizes[0]; i--; ){
      for(j = sizes[1]; j--; ){
         for(k = sizes[2]; k--; ){

            real = get_volume_real_value(*in_vol, i, j, k, 0, 0);
            imag = get_volume_real_value(*in_vol, i, j, k, 1, 0);

            switch (job){
            default:
            case OUTPUT_MAGNITUDE:
               value = sqrt((real * real) + (imag * imag));
               break;

            case OUTPUT_PHASE:
               if(real != 0.0){
                  value = atan(imag / real);
                  }
               else{
                  value = 0.0;
                  }
               break;

            case OUTPUT_MAGLN:
               if(value > 0.1)
                  value = log(sqrt(real * real + imag * imag));
               else
                  value = -2.3;
               break;

            case OUTPUT_MAG10:
               if(value > 0.1)
                  value = log10(sqrt(real * real + imag * imag));
               else
                  value = -1;
               break;

            case OUTPUT_POWER:
               value = (real * real) + (imag * imag);
               break;

            case OUTPUT_REAL:
               value = real;
               break;

            case OUTPUT_IMAG:
               value = imag;
               break;

               }

            if(value < min){
               min = value;
               }
            if(value > max){
               max = value;
               }

            set_volume_real_value(*out_vol, i, j, k, 0, 0, value);
            }
         }
      }

   set_volume_real_range(*out_vol, min, max);
   return (OK);
   }

/* ----------------------------- MNI Header -----------------------------------
@NAME       : fft_volume.c
@INPUT      : data - a pointer to a volume_struct of data
              inverse_flg = TRUE if inverse fft to be done.
@OUTPUT     : data -
@RETURNS    : status variable - OK or ERROR.
@DESCRIPTION: this procedure uses numerical recipes routines
              to do an N-dimensional fourier transform.
@METHOD     : 
@GLOBALS    : 
@CALLS      : fftw
@CREATED    : Thu Nov  4 11:16:54 EST 1993 Louis
@MODIFIED   : $Log: fft_support.c,v $
@MODIFIED   : Revision 1.2  2002-11-21 01:48:18  rotor
@MODIFIED   : * Changed complex_vector to MIvector_dimension
@MODIFIED   : * large reorganisations to the 2D fft calculations
@MODIFIED   :      to the point that they should be correct now.
@MODIFIED   :
@MODIFIED   : Revision 1.1  2002/09/17 23:44:51  rotor
@MODIFIED   : ----------------------------------------------------------------------
@MODIFIED   : Initial entry of mincfft to CVS (to perhaps replace louis's old NR
@MODIFIED   :    version of mincfft
@MODIFIED   :
@MODIFIED   : Committing in .
@MODIFIED   :
@MODIFIED   : Added Files:
@MODIFIED   :  Makefile fft_support.c fft_support.h mincfft.c
@MODIFIED   : ----------------------------------------------------------------------
@MODIFIED   :
 * Revision 1.1  93/11/05  14:19:52  louis
 * Initial revision
 * 
---------------------------------------------------------------------------- */
Status fft_volume(Volume data, int inverse_flg, int dim, int centre)
{
/* From the fftw FAQ                                                            */
/* 3.5 How can I make FFTW put the origin at the center of its output?          */
/*                                                                              */
/* For human viewing of a spectrum, it is often convenient to put the origin    */
/* in frequency space at the center of the output array, rather than in the     */
/* zero-th element (the default in FFTW).  If all of the dimensions of your     */
/* array are even, you can accomplish this by simply multiplying each element   */
/* of the input array by (-1)^(i + j + ...)                                     */
   
   Status status;
   
   switch(dim){
   case 2:
      status = fft_volume_2d(data, inverse_flg, centre);
      break;
      
   case 3:
      status = fft_volume_3d(data, inverse_flg, centre);
      break;
      
   default:
      fprintf(stderr, "Glark! I canna do %d dimensional FFT's yet!\n", dim);
      status = ERROR;
      break;
      }
   
   return status;
   }

/* do a 2d fft on a 3d volume (slice by slice) */
Status fft_volume_2d(Volume data, int inverse_flg, int centre)
{

   int      i, j, k;
   int      sizes[4];
   Real     value, factor, divisor;
   progress_struct progress;

   fftw_complex *fftw_data;
   fftw_complex *fftw_data_ptr;
   fftwnd_plan p;
   
   get_volume_sizes(data, sizes);
   initialize_progress_report(&progress, FALSE, sizes[0], "FFT");

   /* set up tmp data store */
   fftw_data = (fftw_complex *) malloc(sizes[1] * sizes[2] * sizeof(fftw_complex));
   
   /* set up the 2D FFTW plan */
   p = fftw2d_create_plan(sizes[1], sizes[2],
                          (inverse_flg) ? FFTW_BACKWARD : FFTW_FORWARD,
                          FFTW_ESTIMATE | FFTW_IN_PLACE);
                             
   /* for each slice */
   for(i = sizes[0]; i--; ){
      
      /* do the super-funky shift to centre calculation if required */
      fftw_data_ptr = fftw_data;
      factor = 1.0;
      for(j = sizes[1]; j--; ){
         for(k = sizes[2]; k--; ){
            if(centre){
               factor = pow(-1.0, j + k);
               }

            GET_VOXEL_4D(value, data, i, j, k, 0);
            c_re(*fftw_data_ptr) = (fftw_real) (value * factor);

            GET_VOXEL_4D(value, data, i, j, k, 1);
            c_im(*fftw_data_ptr) = (fftw_real) (value * factor);

            fftw_data_ptr++;
            }
         }

      /* do the FFT */
      fftwnd_one(p, fftw_data, NULL);

      /* put the data back */
      divisor = (inverse_flg) ? sizes[1] * sizes[2] : 1.0;

      fftw_data_ptr = fftw_data;
      for(j = sizes[1]; j--; ){
         for(k = sizes[2]; k--; ){
            SET_VOXEL_4D(data, i, j, k, 0, (Real) c_re(*fftw_data_ptr) / divisor);
            SET_VOXEL_4D(data, i, j, k, 1, (Real) c_im(*fftw_data_ptr) / divisor);
            fftw_data_ptr++;
            }
         }
      
      update_progress_report(&progress, sizes[0] - i);
      }

   /* be tidy */
   fftwnd_destroy_plan(p);
   free(fftw_data);
   terminate_progress_report(&progress);

   return (OK);
   }

/* do a 3d fft on a 3d volume */
Status fft_volume_3d(Volume data, int inverse_flg, int centre){
   int      i, j, k;
   int      sizes[4];
   Real     value, factor, divisor;
   progress_struct progress;

   fftw_complex *fftw_data;
   fftw_complex *fftw_data_ptr;
   fftwnd_plan p;
   
   get_volume_sizes(data, sizes);
   initialize_progress_report(&progress, FALSE, sizes[0]*3, "FFT");

   /* set up tmp data store */
   fftw_data = (fftw_complex *) malloc(sizes[0] * sizes[1] * sizes[2] * sizeof(fftw_complex));
   
   /* do the super-funky shift to centre calculation if required */
   fftw_data_ptr = fftw_data;
   factor = 1.0;
   for(i = sizes[0]; i--; ){
      for(j = sizes[1]; j--; ){
         for(k = sizes[2]; k--; ){
            if(centre){
               factor = pow(-1.0, i + j + k);
               }

            GET_VOXEL_4D(value, data, i, j, k, 0);
            c_re(*fftw_data_ptr) = (fftw_real) (value * factor);

            GET_VOXEL_4D(value, data, i, j, k, 1);
            c_im(*fftw_data_ptr) = (fftw_real) (value * factor);

            fftw_data_ptr++;
            }
         }
      update_progress_report(&progress, sizes[0] - i);
      }

   /* do the FFT */
   p = fftw3d_create_plan(sizes[0], sizes[1], sizes[2],
                          (inverse_flg) ? FFTW_BACKWARD : FFTW_FORWARD,
                          FFTW_ESTIMATE | FFTW_IN_PLACE);

   fftwnd_one(p, fftw_data, NULL);
   update_progress_report(&progress, sizes[0] * 2);

   /* put the data back */
   divisor = (inverse_flg) ? sizes[0] * sizes[1] * sizes[2] : 1.0;

   fftw_data_ptr = fftw_data;
   for(i = sizes[0]; i--; ){
      for(j = sizes[1]; j--; ){
         for(k = sizes[2]; k--; ){
            SET_VOXEL_4D(data, i, j, k, 0, (Real) c_re(*fftw_data_ptr) / divisor);
            SET_VOXEL_4D(data, i, j, k, 1, (Real) c_im(*fftw_data_ptr) / divisor);
            fftw_data_ptr++;
            }
         }
      update_progress_report(&progress, (sizes[0]*3) - i);
      }

   /* be tidy */
   fftwnd_destroy_plan(p);
   free(fftw_data);
   terminate_progress_report(&progress);

   return (OK);
   }

