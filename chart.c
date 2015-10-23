/*
 * Copyright 2012 Alexey Berezhok (alexey_com@ukr.net, bayrepo.info@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * chart.c
 *
 *  Created on: Aug 6, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <apr_general.h>
#include <apr_pools.h>
#include <apr_tables.h>
#include <apr_strings.h>

#include <gd.h>
#include <gdfonts.h>

#include "chart.h"

chart_color preColor[7] =
  {
    { 42, 180, 0, 68, 138, 1 },
    { 255, 124, 94, 188, 67, 39 },
    { 231, 207, 0, 155, 139, 0 },
    { 61, 157, 255, 22, 111, 202 },
    { 217, 0, 0, 144, 0, 0 },
    { 255, 201, 107, 242, 154, 0 },
    { 104, 232, 255, 29, 188, 218 } };

//---------------------Debug-----------------------------------------------
void
chart_print_data(apr_array_header_t *data ,char *name)
{
  long i;
  FILE *fp;
  fp = fopen(name, "w");
  if(fp!=NULL){
    for (i = 0; i < data->nelts; i++)
    {
      chart_data_percent *s = ((chart_data_percent **) data->elts)[i];
      fprintf(fp,"%ld) date %d, value %f\n", i, s->date, s->value);
    }
    fprintf(fp, "----------------------\n");
    fclose(fp);
  }
}

void
chart_print_data_pie(apr_array_header_t *data)
{
  long i;
  for (i = 0; i < data->nelts; i++)
    {
      chart_data_pie *s = ((chart_data_pie **) data->elts)[i];
      printf("%ld) name %s, value %f\n", i, s->name, s->value);
    }
  printf("----------------------\n");
}

void
chart_write_to_file(gdImagePtr im, char *file_name)
{
  FILE *fp;
  fp = fopen(file_name, "w");
  if (fp)
    {
      gdImageGif(im, fp);
      fclose(fp);
    }
}

void
printresult(double *a, int K)
{
  //print polynom parameters


  int i = 0;
  printf("\n");
  for (i = 0; i < K + 1; i++)
    {
      printf("a[%d] = %g\n", i, a[i]);
    }
}


//---------------------Approximation---------------------------------------

int
count_num_lines(apr_array_header_t *data)
{
  return data->nelts;
}

void
allocmatrix(apr_pool_t *p, double **a, double **b, double **x, double **y,
    double ***sums, int K, int N)
{
  //allocate memory for matrixes


  int i, j, k;
  *a = apr_palloc(p, sizeof(double) * (K + 1));
  *b = apr_palloc(p, sizeof(double) * (K + 1));
  *x = apr_palloc(p, sizeof(double) * N);
  *y = apr_palloc(p, sizeof(double) * N);
  *sums = apr_palloc(p, sizeof(double*) * (K + 1));

  for (i = 0; i < (K + 1); i++)
    {
      (*sums)[i] = apr_palloc(p, sizeof(double) * (K + 1));
    }
  for (i = 0; i < (K + 1); i++)
    {
      (*a)[i] = 0;
      (*b)[i] = 0;
      for (j = 0; j < (K + 1); j++)
        {
          (*sums)[i][j] = 0;
        }
    }
  for (k = 0; k < N; k++)
    {
      (*x)[k] = 0;
      (*y)[k] = 0;
    }

}

void
readmatrix(apr_array_header_t *data, double **a, double **b, double **x,
    double **y, double ***sums, int K, int N)
{
  int i = 0, j = 0, k = 0;
  //read x, y matrixes from input file
  for (k = 0; k < data->nelts; k++)
    {
      chart_data_percent *s = ((chart_data_percent **) data->elts)[k];
      chart_data_percent *s_0 = ((chart_data_percent **) data->elts)[0];
      (*x)[k] = (double) (s->date - s_0->date);
      (*y)[k] = s->value;
    }
  //init square sums matrix
  for (i = 0; i < (K + 1); i++)
    {
      for (j = 0; j < (K + 1); j++)
        {
          (*sums)[i][j] = 0;
          for (k = 0; k < N; k++)
            {
              (*sums)[i][j] += pow((*x)[k], i + j);
            }
        }

    }
  //init free coefficients column
  for (i = 0; i < K + 1; i++)
    {
      for (k = 0; k < N; k++)
        {
          (*b)[i] += pow((*x)[k], i) * (*y)[k];
        }
    }
}

void
diagonal(double **a, double **b, double **x, double **y, double ***sums, int K,
    int N)
{
  int i, j, k;
  double temp = 0;
  for (i = 0; i < (K + 1); i++)
    {

      if ((*sums)[i][i] == 0)
        {
          for (j = 0; j < (K + 1); j++)
            {
              if (j == i)
                continue;
              if ((*sums)[j][i] != 0 && (*sums)[i][j] != 0)
                {
                  for (k = 0; k < (K + 1); k++)
                    {
                      temp = (*sums)[j][k];
                      (*sums)[j][k] = (*sums)[i][k];

                      (*sums)[i][k] = temp;
                    }
                  temp = (*b)[j];
                  (*b)[j] = (*b)[i];
                  (*b)[i] = temp;
                  break;
                }
            }
        }
    }
}

int
result_approx(double **a, double **b, double **x, double **y, double ***sums,
    int K, int N)
{
  //process rows
  int i, j, k;
  for (k = 0; k < (K + 1); k++)
    {
      for (i = k + 1; i < (K + 1); i++)
        {
          if ((*sums)[k][k] == 0)
            {
              return -1;
            }
          double M = (*sums)[i][k] / (*sums)[k][k];

          for (j = k; j < K + 1; j++)
            {
              (*sums)[i][j] -= M * (*sums)[k][j];
            }
          (*b)[i] -= M * (*b)[k];
        }
    }
  for (i = (K + 1) - 1; i >= 0; i--)
    {
      double s = 0;
      for (j = i; j < K + 1; j++)
        {

          s = s + (*sums)[i][j] * (*a)[j];
        }
      (*a)[i] = ((*b)[i] - s) / (*sums)[i][i];
    }

  return 0;
}

//-------------------------------------------------------------------------

#define chart_get_color_at(im, x, y) ((im)->trueColor ? gdImageGetTrueColorPixel(im, x, y) : \
    gdImageGetPixel(im ,x, y))
#define min(x, y) (x>y)?y:x
#define max(x, y) (x>y)?x:y
#define chart_color_for_index(v, im, c) v[0]=gdImageRed(im, c); \
   v[1]=gdImageGreen(im, c); \
   v[2]=gdImageBlue(im, c)

void
imagesmoothline(gdImagePtr im, int x1, int y1, int x2, int y2, int color)
{
  int colors[3], tempcolors[3];
  chart_color_for_index(colors, im, color);
  if (x1 == x2)
    {
      gdImageLine(im, x1, y1, x2, y2, color);
    }
  else
    {
      double m = (double) (y2 - y1) / (double) (x2 - x1);
      double b = (double) y1 - m * (double) x1;
      if (abs(m) <= 1)
        {
          int x = min ( x1 , x2 );
          int endx = max ( x1 , x2 );
          while (x <= endx)
            {
              double ya, yb, y = m * (double) x + b;
              if (y == floor(y))
                {
                  ya = 1;
                }
              else
                {
                  ya = y - floor(y);
                }
              yb = ceil(y) - y;
              chart_color_for_index(tempcolors, im, chart_get_color_at(im, x , (int)floor ( y )));
              tempcolors[0] = (int) (tempcolors[0] * ya + colors[0] * yb);
              tempcolors[1] = (int) (tempcolors[1] * ya + colors[1] * yb);
              tempcolors[2] = (int) (tempcolors[2] * ya + colors[2] * yb);
              if (gdImageColorExact(im, tempcolors[0], tempcolors[1],
                  tempcolors[2]))
                {
                  gdImageColorAllocate(im, tempcolors[0], tempcolors[1],
                      tempcolors[2]);
                }
              gdImageSetPixel(im, x, (int) floor(y), gdImageColorExact(im,
                  tempcolors[0], tempcolors[1], tempcolors[2]));
              chart_color_for_index(tempcolors, im, chart_get_color_at(im, x , (int)ceil ( y )));
              tempcolors[0] = (int) (tempcolors[0] * yb + colors[0] * ya);
              tempcolors[1] = (int) (tempcolors[1] * yb + colors[1] * ya);
              tempcolors[2] = (int) (tempcolors[2] * yb + colors[2] * ya);
              if (gdImageColorExact(im, tempcolors[0], tempcolors[1],
                  tempcolors[2]))
                {
                  gdImageColorAllocate(im, tempcolors[0], tempcolors[1],
                      tempcolors[2]);
                }
              gdImageSetPixel(im, x, (int) ceil(y), gdImageColorExact(im,
                  tempcolors[0], tempcolors[1], tempcolors[2]));
              x++;
            }
        }
      else
        {
          int y = min ( y1 , y2 );
          int endy = max ( y1 , y2 );
          while (y <= endy)
            {
              double xa, xb, x = ((double) y - b) / m;
              if (x == floor(x))
                {
                  xa = 1;
                }
              else
                {
                  xa = x - floor(x);
                }
              xb = ceil(x) - x;
              chart_color_for_index(tempcolors, im, chart_get_color_at(im, (int)floor(x) , y ));
              tempcolors[0] = (int) (tempcolors[0] * xa + colors[0] * xb);
              tempcolors[1] = (int) (tempcolors[1] * xa + colors[1] * xb);
              tempcolors[2] = (int) (tempcolors[2] * xa + colors[2] * xb);
              if (gdImageColorExact(im, tempcolors[0], tempcolors[1],
                  tempcolors[2]))
                {
                  gdImageColorAllocate(im, tempcolors[0], tempcolors[1],
                      tempcolors[2]);
                }
              gdImageSetPixel(im, (int) floor(x), y, gdImageColorExact(im,
                  tempcolors[0], tempcolors[1], tempcolors[2]));
              chart_color_for_index(tempcolors, im, chart_get_color_at(im, (int)ceil(x) , y ));
              tempcolors[0] = (int) (tempcolors[0] * xb + colors[0] * xa);
              tempcolors[1] = (int) (tempcolors[1] * xb + colors[1] * xa);
              tempcolors[2] = (int) (tempcolors[2] * xb + colors[2] * xa);
              if (gdImageColorExact(im, tempcolors[0], tempcolors[1],
                  tempcolors[2]))
                {
                  gdImageColorAllocate(im, tempcolors[0], tempcolors[1],
                      tempcolors[2]);
                }
              gdImageSetPixel(im, (int) ceil(x), y, gdImageColorExact(im,
                  tempcolors[0], tempcolors[1], tempcolors[2]));
              y++;

            }
        }
    }
}

int
chart_get_min(apr_array_header_t *data)
{
  long i;
  int min = -1;
  for (i = 0; i < data->nelts; i++)
    {
      chart_data_percent *s = ((chart_data_percent **) data->elts)[i];
      if (min == -1)
        min = s->date;
      else if (min > s->date)
        min = s->date;
    }
  return min;
}

int
chart_get_max(apr_array_header_t *data)
{
  long i;
  int max = -1;
  for (i = 0; i < data->nelts; i++)
    {
      chart_data_percent *s = ((chart_data_percent **) data->elts)[i];
      if (max == -1)
        max = s->date;
      else if (max < s->date)
        max = s->date;
    }
  return max;
}

double
chart_get_max_value(apr_array_header_t *data)
{
  long i;
  double max = 0;
  for (i = 0; i < data->nelts; i++)
    {
      chart_data_percent *s = ((chart_data_percent **) data->elts)[i];
      if (max == -1)
        max = s->value;
      else if (max < s->value)
        max = s->value;
    }
  return max;
}

double
chart_get_average(apr_array_header_t *data, int cur, int next, long *index)
{
  long i = 0;
  double avg = 0;
  int counter = 0;
  for (i = *index; i < data->nelts; i++)
    {
      chart_data_percent *s = ((chart_data_percent **) data->elts)[i];
      if ((s->date >= cur) && (s->date < next))
        {
          avg += s->value;
          counter++;
        }
      if (s->date >= next)
        {
          *index = i;
          break;
        }
    }
  if (counter > 0)
    {
      return avg / (double) counter;
    }
  else
    {
      return avg;
    }
}

double
chart_get_value(apr_array_header_t *data, int cur, long *index)
{
  long i;
  for (i = *index; i < data->nelts; i++)
    {
      chart_data_percent *s = ((chart_data_percent **) data->elts)[i];
      if (s->date == cur)
        {
          *index = i + 1;
          return s->value;
        }
      if (s->date > cur)
        return 0.0;
    }
  return 0.0;
}

apr_array_header_t *
chart_fill_data(apr_array_header_t *data, int min, int max)
{
  apr_array_header_t *new_data = apr_array_make(data->pool, 1,
      sizeof(chart_data_percent *));
  int i;
  long ii = 0;
  for (i = min; i <= max; i++)
    {
      chart_data_percent *ptr = apr_palloc(data->pool,
          sizeof(chart_data_percent));
      ptr->date = i;
      ptr->value = chart_get_value(data, i, &ii);
      *(chart_data_percent **) apr_array_push(new_data) = ptr;
    }
  return new_data;
}

void
chart_get_sized_value(char *buffer, double value)
{
  long i_value = 1000000000;
  long vl = (int) ceil(value);
  if (vl / i_value)
    {
      sprintf(buffer, "%6.1fG", value / (double)i_value);
      return;
    }
  i_value = i_value / 1000;
  if (vl / i_value)
    {
      sprintf(buffer, "%6.1fM", value / (double)i_value);
      return;
    }
  i_value = i_value / 1000;
  if (vl / i_value)
    {
      sprintf(buffer, "%6.1fK", value / (double)i_value);
      return;
    }
  sprintf(buffer, "%6.1f", value);
  return;
}

apr_array_header_t *
chart_approximate_data(apr_array_header_t *data, int width, int height)
{
  apr_array_header_t *filled_data, *new_data = apr_array_make(data->pool, 1,
      sizeof(chart_data_percent *));

  int max_value, min_value, i;

  min_value = chart_get_min(data);
  max_value = chart_get_max(data);
  if ((max_value - min_value) < height)
    {
      max_value = min_value + height;
    }
  filled_data = chart_fill_data(data, min_value, max_value);
  float dx = (float) (max_value - min_value) / (float) height;

  long ii = 0;
  for (i = 0; i < height; i++)
    {
      chart_data_percent *ptr = apr_palloc(data->pool,
          sizeof(chart_data_percent));
      ptr->date = min_value + (int) ceil(((float) i) * dx);
      if (i == (height - 1))
        ptr->value = chart_get_average(filled_data, min_value + (int) ceil(
            ((float) i) * dx), min_value + (int) ceil(((float) (i + 2)) * dx),
            &ii);
      else
        ptr->value = chart_get_average(filled_data, min_value + (int) ceil(
            ((float) i) * dx), min_value + (int) ceil(((float) (i + 1)) * dx),
            &ii);
      *(chart_data_percent **) apr_array_push(new_data) = ptr;
    }

  return new_data;
}

#define _SPX 50
#define _SPY 50
#define _EPX 50
#define _EPY 50

gdImagePtr
chart_create_bg_full(gdImagePtr im, int width, int height)
{
  int gray1, gray2, gray3, white;
  int rw = width - 1, rh = height - 1, spx = _SPX, spy = _SPY, epx = _EPX, epy =
      _EPY;
  epy = height - ((height - spy - epy - 4) / 10 * 10) - spy - 4;
  gray1 = gdImageColorAllocate(im, 240, 240, 240);
  gray2 = gdImageColorAllocate(im, 226, 226, 226);
  gray3 = gdImageColorAllocate(im, 235, 235, 235);
  white = gdImageColorAllocate(im, 255, 255, 255);
  gdImageFilledRectangle(im, 0, 0, rw, rh, gray1);
  gdImageRectangle(im, spx, spy, rw - epx, rh - epy, gray3);
  gdImageRectangle(im, spx + 1, spy + 1, rw - epx - 1, rh - epy - 1, gray2);
  gdImageFilledRectangle(im, spx + 2, spy + 2, rw - epx - 2, rh - epy - 2,
      white);

  return im;
}

gdImagePtr
chart_create_bg(gdImagePtr im, int width, int height)
{
  int /*gray1, gray2, gray3, white, */ gray4, gray5, gray6;
  int rw = width - 1, rh = height - 1, spx = _SPX, spy = _SPY, epx = _EPX, epy =
      _EPY;
  epy = height - ((height - spy - epy - 4) / 10 * 10) - spy - 4;
  /*gray1 = gdImageColorAllocate(im, 240, 240, 240);
  gray2 = gdImageColorAllocate(im, 226, 226, 226);
  gray3 = gdImageColorAllocate(im, 235, 235, 235);
  white = gdImageColorAllocate(im, 255, 255, 255);*/
  gray4 = gdImageColorAllocate(im, 230, 230, 230);
  gray5 = gdImageColorAllocate(im, 150, 150, 150);
  gray6 = gdImageColorAllocate(im, 195, 195, 195);
  //gdImageFilledRectangle(im, 0, 0, rw, rh, gray1);
  //gdImageRectangle(im, spx, spy, rw - epx, rh - epy, gray3);
  //gdImageRectangle(im, spx + 1, spy + 1, rw - epx - 1, rh - epy - 1, gray2);
  //gdImageFilledRectangle(im, spx + 2, spy + 2, rw - epx - 2, rh - epy - 2,
  //    white);
  gdImageLine(im, spx + 2, spy + 1, spx + 2, rh - epy - 2, gray5);
  gdImageLine(im, spx + 2, rh - epy - 2, rw - epx - 1, rh - epy - 2, gray5);
  int i, dx = 20, dy = (height - (spy + epy)) / 10;
  for (i = 0; i < (width - ((spx + 2) + (epx + 2)) - dx); i += dx)
    {
      gdImageDashedLine(im, spx + 2 + dx + i, spy + 2, spx + 2 + dx + i, rh
          - epy - 2, gray4);
      gdImageLine(im, spx + 2 + dx + i, rh - epy - 2, spx + 2 + dx + i, rh
          - epy + 2, gray5);
    }
  for (i = (rh - epy - 1 - dy); i >= (spy + 2); i -= dy)
    {
      gdImageDashedLine(im, spx + 2, i, rw - epx - 2, i, gray4);
      if (!(i % 2))
        gdImageLine(im, spx + 2, i, spx - 2, i, gray5);
      else
        {
          gdImageLine(im, spx + 2, i, spx - 2, i, gray6);
          gdImageLine(im, spx + 2, i + 1, spx - 2, i + 1, gray6);
        }
    };
  return im;
}

void
chart_calculate_date(int dt, chart_time_interval *buffer)
{
  int days = dt / (60 * 60 * 24);
  dt = dt - days * (60 * 60 * 24);
  int hours = dt / (60 * 60);
  dt = dt - hours * (60 * 60);
  int minutes = dt / 60;
  dt = dt - minutes * 60;
  int seconds = dt;

  buffer->days = days;
  buffer->hours = hours;
  buffer->minutes = minutes;
  buffer->seconds = seconds;
}

gdImagePtr
chart_create_legend(gdImagePtr im, int width, int height,
    apr_array_header_t *data, int flags)
{
  int show_date = flags & CHART_DATE;
  int gray5, gray3, gray1, blue, yellow, orang, black;
  char buffer[256], add_buffer[256];
  gray5 = gdImageColorAllocate(im, 150, 150, 150);
  gray3 = gdImageColorAllocate(im, 215, 215, 215);
  gray1 = gdImageColorAllocate(im, 226, 226, 226);
  blue = gdImageColorAllocate(im, 112, 157, 232);
  yellow = gdImageColorAllocate(im, 252, 254, 188);
  orang = gdImageColorAllocate(im, 212, 195, 100);
  black = gdImageColorAllocate(im, 0, 0, 0);
  double max_value = chart_get_max_value(data);
  //int min = chart_get_min(data), max = chart_get_max(data);
  int max_value_i = (int) ceil(max_value);
  gdFontPtr fptr = gdFontGetSmall();
  int rw = width - 1, rh = height - 1, spx = _SPX, spy = _SPY, epx = _EPX, epy =
      _EPY;
  epy = height - ((height - spy - epy - 4) / 10 * 10) - spy - 4;
  int i, dx = 20, dy = (height - (spy + epy)) / 10;
  int j = 0;
  for (i = 0; i < (width - ((spx + 2) + (epx + 2))); i += dx)
    {
      gdPoint pt[5];
      pt[0].x = spx + 2 + i - 6;
      pt[0].y = rh - 1;
      pt[1].x = spx + 2 + i - 6;
      pt[1].y = rh - 48;
      pt[2].x = spx + 2 + i - 6 + (dx - 3) / 2 - 2;
      pt[2].y = rh - 52;
      pt[3].x = spx + 2 + i - 6 + dx - 3;
      pt[3].y = rh - 48;
      pt[4].x = spx + 2 + i - 6 + dx - 3;
      pt[4].y = rh - 1;
      gdPoint pt2[5];
      pt2[0].x = spx + 2 + i - 5;
      pt2[0].y = rh - 1;
      pt2[1].x = spx + 2 + i - 5;
      pt2[1].y = rh - 49;
      pt2[2].x = spx + 2 + i - 5 + (dx - 3) / 2 - 2;
      pt2[2].y = rh - 53;
      pt2[3].x = spx + 2 + i - 5 + dx - 3;
      pt2[3].y = rh - 49;
      pt2[4].x = spx + 2 + i - 5 + dx - 3;
      pt2[4].y = rh - 1;
      gdImageFilledPolygon(im, pt2, 5, orang);
      gdImageFilledPolygon(im, pt, 5, yellow);
      chart_data_percent *s = ((chart_data_percent**) data->elts)[j];
      j += 20;
      if (show_date)
        {
          struct tm *timeinfo;
          time_t local_time = s->date;
          timeinfo = gmtime(&local_time);
          strftime(buffer, 256, "%m.%d", timeinfo);
          strftime(add_buffer, 256, "%H:%M:%S", timeinfo);

          gdImageStringUp(im, fptr, spx + 2 + i - 9, rh - 1, (unsigned char *)buffer, blue);
          gdImageStringUp(im, fptr, spx + 2 + i, rh - 1, (unsigned char *)add_buffer, gray5);
        }
      else
        {
          sprintf(buffer, "%8d", s->date);
          gdImageStringUp(im, fptr, spx + 2 + i - 6, rh - 1, (unsigned char *)buffer, gray5);
        }
    }
  double j1 = 0;
  for (i = (rh - epy - 1 - dy); i >= (spy + 2); i -= dy)
    {
      j1 += (double)max_value_i / 10.0;
      if (flags & CHART_SHOW_NO_PERSENT)
        {
          sprintf(buffer, "%6.1f", j1);
        }
      else
        {
          if (flags & CHART_SHOW_UNITS)
            {
               chart_get_sized_value(buffer, j1);
            }
          else
            {
              sprintf(buffer, "%5.1f%%", j1);
            }
        }
      gdImageString(im, fptr, spx + 2 - 47, i - 7, (unsigned char *)buffer, gray1);
      gdImageString(im, fptr, spx + 2 - 47, i - 6, (unsigned char *)buffer, gray3);
      gdImageString(im, fptr, spx + 2 - 48, i - 7, (unsigned char *)buffer, gray5);
    };

  chart_data_percent *s1 = ((chart_data_percent**) data->elts)[0];
  chart_data_percent *s2 = ((chart_data_percent**) data->elts)[20];
  int ddy = s2->date - s1->date;
  if (show_date)
    {
      gdImageLine(im, rw - 2 * dx, rh - 50, rw - dx, rh - 50, black);
      gdImageLine(im, rw - 2 * dx, rh - 48, rw - 2 * dx, rh - 52, black);
      gdImageLine(im, rw - dx, rh - 48, rw - dx, rh - 52, black);
      chart_time_interval ti;
      chart_calculate_date(ddy, &ti);
      sprintf(buffer, "%5d", ddy);
      int dup = 47;
      if (ti.days)
        {
          sprintf(buffer, "ds %d", ti.days);
          gdImageString(im, fptr, rw - 2 * dx - 10, rh - dup, (unsigned char *)buffer, black);
          dup -= 10;
        }
      if (ti.hours)
        {
          sprintf(buffer, "hr %d", ti.hours);
          gdImageString(im, fptr, rw - 2 * dx - 10, rh - dup, (unsigned char *)buffer, black);
          dup -= 10;
        }
      if (ti.minutes)
        {
          sprintf(buffer, "mins %d", ti.minutes);
          gdImageString(im, fptr, rw - 2 * dx - 10, rh - dup, (unsigned char *)buffer, black);
          dup -= 10;
        }
      if (ti.seconds)
        {
          sprintf(buffer, "sec %d", ti.seconds);
          gdImageString(im, fptr, rw - 2 * dx - 10, rh - dup, (unsigned char *)buffer, black);
          dup -= 10;
        }
    }
  else
    {
      gdImageLine(im, rw - 2 * dx, rh - 40, rw - dx, rh - 40, black);
      gdImageLine(im, rw - 2 * dx, rh - 38, rw - 2 * dx, rh - 42, black);
      gdImageLine(im, rw - dx, rh - 38, rw - dx, rh - 42, black);
      sprintf(buffer, "%5d", ddy);
      gdImageString(im, fptr, rw - 2 * dx - 10, rh - 37, (unsigned char *)buffer, black);
    }
  return im;
}

gdImagePtr
chart_create_name(gdImagePtr im, char *name, int width, int height)
{
  int spy = _SPY;
  int black, gray, gray1;
  black = gdImageColorAllocate(im, 35, 37, 27);
  gray = gdImageColorAllocate(im, 200, 200, 200);
  gray1 = gdImageColorAllocate(im, 210, 210, 210);
  gdFontPtr fptr = gdFontGetSmall();
  int l = strlen(name) * 8 / 2;
  gdImageString(im, fptr, width / 2 - l + 1, spy / 2 - 8, (unsigned char *)name, gray1);
  gdImageString(im, fptr, width / 2 - l + 1, spy / 2 - 7, (unsigned char *)name, gray);
  gdImageString(im, fptr, width / 2 - l, spy / 2 - 8, (unsigned char *)name, black);
  return im;
}

gdImagePtr
chart_create_bars(gdImagePtr im, apr_array_header_t *data, char *graph_name,
    int flags, int width, int height, int gamma)
{
  int green, green2;
  apr_array_header_t *new_line = chart_approximate_data(data, height, width
      - _SPX - _EPX - 4);

  im = gdImageCreate(width, height);
  green = gdImageColorAllocate(im, preColor[gamma].r, preColor[gamma].g,
      preColor[gamma].b);
  green2 = gdImageColorAllocate(im, preColor[gamma].r1, preColor[gamma].g1,
      preColor[gamma].b1);

  im = chart_create_bg_full(im, width, height);

  im = chart_create_bg(im, width, height);
  im = chart_create_legend(im, width, height, new_line, flags);
  im = chart_create_name(im, graph_name, width, height);

  int rh = height - 1, spx = _SPX, spy = _SPY,  epy =
      _EPY;
  epy = height - ((height - spy - epy - 4) / 10 * 10) - spy - 4;

  int max_value_i = (int) ceil(chart_get_max_value(new_line));
  double dy = (height - spy - epy - 4) / (double) max_value_i;

  long i;
  for (i = 1; i < new_line->nelts; i++)
    {
      chart_data_percent *s = ((chart_data_percent **) new_line->elts)[i];
      if (s->value > 0)
        {
          gdImageLine(im, spx + 2 + i, rh - epy - 2
              - (int) floor(s->value * dy), spx + 2 + i, rh - epy - 3, green);
          if (i != (new_line->nelts - 1))
            {
              gdImageLine(im, spx + 2 + i + 1, rh - epy - 2 - (int) floor(
                  s->value * dy), spx + 2 + i + 1, rh - epy - 3, green2);
            }

        }
    }

  return im;

}

apr_array_header_t *
chart_get_genetic(apr_array_header_t *data)
{
  /*TODO
   * Думаю может сюда вставить аппроксимацию gsl
   * */
  return data;
}

#define GAMMA_NEXT(g) (((g+1)==CHART_MAX_COLOR)?0:(g+1))

gdImagePtr
chart_create_polyline(gdImagePtr im, apr_array_header_t *data,
    char *graph_name, int flags, int width, int height, int gamma)
{
  int green /*, green2*/, red;
  apr_array_header_t *new_line = chart_approximate_data(data, height, width
      - _SPX - _EPX - 4);

  double *a, *b, *x, *y, **sums;

  int N, K = 50;
  N = count_num_lines(new_line);
  allocmatrix(new_line->pool, &a, &b, &x, &y, &sums, K, N);
  readmatrix(new_line, &a, &b, &x, &y, &sums, K, N);
  diagonal(&a, &b, &x, &y, &sums, K, N);
  result_approx(&a, &b, &x, &y, &sums, K, N);

  new_line = chart_get_genetic(new_line);

  im = gdImageCreate(width, height);
  green = gdImageColorAllocate(im, preColor[gamma].r, preColor[gamma].g,
      preColor[gamma].b);
  //green2 = gdImageColorAllocate(im, preColor[gamma].r1, preColor[gamma].g1,
  //    preColor[gamma].b1);
  red = gdImageColorAllocate(im, preColor[GAMMA_NEXT(gamma)].r,
      preColor[GAMMA_NEXT(gamma)].g, preColor[GAMMA_NEXT(gamma)].b);

  im = chart_create_bg_full(im, width, height);

  im = chart_create_bg(im, width, height);
  im = chart_create_legend(im, width, height, new_line, flags);
  im = chart_create_name(im, graph_name, width, height);

  int rh = height - 1, spx = _SPX, spy = _SPY,  epy =
      _EPY;
  epy = height - ((height - spy - epy - 4) / 10 * 10) - spy - 4;

  int max_value_i = (int) ceil(chart_get_max_value(new_line));
  double dy = (height - spy - epy - 4) / (double) max_value_i;

  //imagesmoothline(im, 20,20, rw-50, rh-50, green2);

  //return im;

  long i, j;
  for (i = 1; i < new_line->nelts; i++)
    {
      chart_data_percent *s = ((chart_data_percent **) new_line->elts)[i];
      chart_data_percent *s_old = ((chart_data_percent **) new_line->elts)[i
          - 1];

      if ((s->value > 0) || (s_old->value > 0))
        {
          double val1 = (s->value == 0.0) ? 1.0 : s->value;
          double val2 = (s_old->value == 0.0) ? 1.0 : s_old->value;
          gdImageLine(im, spx + 2 + i - 1, rh - epy - 2
              - (int) floor(val2 * dy), spx + 2 + i, rh - epy - 2
              - (int) floor(val1 * dy), green);
          //if (i != (new_line->nelts - 1))
          //  gdImageLine(im, spx + 2 + i, rh - epy - 2 - (int) floor(val2 * dy),
          //      spx + 2 + i + 1, rh - epy - 2 - (int) floor(val1 * dy), green2);
        }

      double xx, yy, yy_old;
      xx = (double) i;
      yy = 0;
      for (j = 0; j < K; j++)
        {
          yy += a[j] * pow(xx, j);
        }
      xx = (double) i - 1;
      yy_old = 0;
      for (j = 0; j < K; j++)
        {
          yy_old += a[j] * pow(xx, j);
        }
      if ((yy_old != 0) && (yy != 0) && ((yy > 0) || (yy_old > 0)))
        {
          if (yy_old < 0)
            yy_old = 1.0;
          if (yy < 0)
            yy = 1.0;
          gdImageLine(im, spx + 2 + i - 1, rh - epy - 2 - (int) floor(yy_old
              * dy), spx + 2 + i, rh - epy - 2 - (int) floor(yy * dy), red);
        }

    }

  return im;

}

apr_array_header_t *
chart_sort_pie_array(apr_array_header_t *data)
{
  apr_array_header_t *new_line = apr_array_make(data->pool, 1,
      sizeof(chart_data_pie *));
  apr_array_header_t *buff_line = apr_array_make(data->pool, 1,
      sizeof(chart_data_pie *));
  apr_array_header_t *buff_line_1 = apr_array_make(data->pool, 1,
      sizeof(chart_data_pie *));
  apr_array_header_t *old_line = data;
  long number = data->nelts;
  long i, j;
  double max = 0.0;

  while (number)
    {
      j = 0;
      max = 0.0;

      for (i = 0; i < old_line->nelts; i++)
        {
          chart_data_pie *s = ((chart_data_pie **) old_line->elts)[i];
          if (max <= s->value)
            {
              max = s->value;
              j = i;
            }
        }

      *(chart_data_pie **) apr_array_push(new_line)
          = ((chart_data_pie **) old_line->elts)[j];

      for (i = 0; i < old_line->nelts; i++)
        {
          chart_data_pie *s = ((chart_data_pie **) old_line->elts)[i];
          if (i != j)
            {
              *(chart_data_pie **) apr_array_push(buff_line_1) = s;
            }
        }

      while (apr_array_pop(buff_line))
        ;
      for (i = 0; i < buff_line_1->nelts; i++)
        {
          chart_data_pie *s = ((chart_data_pie **) buff_line_1->elts)[i];

          *(chart_data_pie **) apr_array_push(buff_line) = s;

        }
      while (apr_array_pop(buff_line_1))
        ;

      old_line = buff_line;
      number--;
    }

  return new_line;
}

apr_array_header_t *
chart_norm_pie_array(apr_array_header_t *data)
{
  apr_array_header_t *new_line = apr_array_make(data->pool, 1,
      sizeof(chart_data_pie *));
  long max = (data->nelts > (CHART_MAX_COLOR - 1)) ? (CHART_MAX_COLOR - 1)
      : data->nelts;
  long i;
  for (i = 0; i < max; i++)
    {
      chart_data_pie *s = ((chart_data_pie **) data->elts)[i];
      *(chart_data_pie **) apr_array_push(new_line) = s;
    }
  double vl = 0;
  for (i = max; i < data->nelts; i++)
    {
      chart_data_pie *s = ((chart_data_pie **) data->elts)[i];
      vl += s->value;
    }
  if (vl != 0.0)
    {
      chart_data_pie * ptr = apr_palloc(data->pool, sizeof(chart_data_pie));
      ptr->name = apr_pstrdup(data->pool, "other");
      ptr->value = vl;
      *(chart_data_pie **) apr_array_push(new_line) = ptr;
    }
  return new_line;
}

gdImagePtr
chart_create_pie(gdImagePtr im, apr_array_header_t *data, char *graph_name,
    int width, int height)
{
  apr_array_header_t *new_line = chart_sort_pie_array(data);
  apr_array_header_t *new_line_norm = chart_norm_pie_array(new_line);
  int colors[CHART_MAX_COLOR], colors_d[CHART_MAX_COLOR];
  im = gdImageCreate(width, height);
  int i;
  for (i = 0; i < CHART_MAX_COLOR; i++)
    {
      colors[i] = gdImageColorAllocate(im, preColor[i].r, preColor[i].g,
          preColor[i].b);
      colors_d[i] = gdImageColorAllocate(im, preColor[i].r / 2, preColor[i].g
          / 2, preColor[i].b / 2);
    }

  int gray1, gray2, gray3, gray4, white, black;
  int rw = width - 1, rh = height - 1;
  gray1 = gdImageColorAllocate(im, 240, 240, 240);
  gray2 = gdImageColorAllocate(im, 150, 150, 150);
  gray3 = gdImageColorAllocate(im, 210, 210, 210);
  gray4 = gdImageColorAllocate(im, 220, 220, 220);
  white = gdImageColorAllocate(im, 255, 255, 255);
  black = gdImageColorAllocate(im, 0, 0, 0);
  gdImageSetAntiAliased(im, gray1);
  gdImageFilledRectangle(im, 0, 0, rw, rh, gray1);
  gdImageRectangle(im, rw - rw / 2 + rw / 15, 40, rw - 20, rh - 20, gray2);
  gdImageFilledRectangle(im, rw - rw / 2 + rw / 15 + 1, 41, rw - 21, rh - 21,
      white);

  double sum = 0.0, sum2 = 0.0;
  double da = 0.0;
  for (i = 0; i < new_line_norm->nelts; i++)
    {
      chart_data_pie *s = ((chart_data_pie **) new_line_norm->elts)[i];
      sum += s->value;
    }
  da = 360 / sum;

  int j = 0;
  for (j = 15; j > 0; j--)
    {
      sum2 = 0;
      for (i = 0; i < new_line_norm->nelts; i++)
        {
          chart_data_pie *s = ((chart_data_pie **) new_line_norm->elts)[i];
          gdImageFilledArc(im, rw / 2 - rw / 4 + 15, rh / 2 + j + 1, rw / 2, rw
              / 4, (int) ceil(sum2 * da), (int) ceil((sum2 + s->value) * da),
              colors_d[i], gdPie);
          sum2 += s->value;
        }
    }
  sum2 = 0;
  for (i = 0; i < new_line_norm->nelts; i++)
    {
      chart_data_pie *s = ((chart_data_pie **) new_line_norm->elts)[i];
      gdImageFilledArc(im, rw / 2 - rw / 4 + 15, rh / 2, rw / 2, rw / 4,
          (int) ceil(sum2 * da), (int) ceil((sum2 + s->value) * da), colors[i],
          gdPie);
      sum2 += s->value;
    }

  gdFontPtr fptr = gdFontGetSmall();

  for (i = 0; i < new_line_norm->nelts; i++)
    {
      chart_data_pie *s = ((chart_data_pie **) new_line_norm->elts)[i];
      char *tmp_data = apr_psprintf(data->pool, "%s (%.2f)", s->name, s->value);
      gdImageString(im, fptr, rw - rw / 2 + rw / 15 + 26, 50 + i * 10, (unsigned char *)tmp_data,
          gray3);
      gdImageString(im, fptr, rw - rw / 2 + rw / 15 + 26, 51 + i * 10, (unsigned char *)tmp_data,
          gray4);
      gdImageString(im, fptr, rw - rw / 2 + rw / 15 + 25, 50 + i * 10, (unsigned char *)tmp_data,
          gray2);
      gdImageRectangle(im, rw - rw / 2 + rw / 15 + 8, 52 + i * 10, rw - rw / 2
          + rw / 15 + 18, 52 + i * 10 + 8, black);
      gdImageFilledRectangle(im, rw - rw / 2 + rw / 15 + 9, 52 + i * 10 + 1, rw
          - rw / 2 + rw / 15 + 17, 52 + i * 10 + 7, colors[i]);
    }

  im = chart_create_name(im, graph_name, width, height);

  return im;

}

