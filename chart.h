/*
 * Copyright 2012 Alexey Berezhok (bayrepo.info@gmail.com)
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
 * chart.h
 *
 *  Created on: Aug 10, 2011
 *  Author: SKOREE
 *  E-mail: a@bayrepo.ru
 */

#ifndef CHART_H_
#define CHART_H_

#define CHART_NO_DATE 0
#define CHART_DATE 1
#define CHART_SHOW_NO_PERSENT 2
#define CHART_SHOW_UNITS 4

typedef struct _chart_color
{
  int r;
  int g;
  int b;
  int r1;
  int g1;
  int b1;
} chart_color;

typedef struct _chart_data_percent
{
  int date;
  double value;
} chart_data_percent;

typedef struct _chart_time_interval
{
  int days;
  int hours;
  int minutes;
  int seconds;
} chart_time_interval;

typedef struct _chart_data_pie
{
  char *name;
  double value;
} chart_data_pie;

#define CHART_COLOR_GREEN 0
#define CHART_COLOR_RED 1
#define CHART_COLOR_YELLOW 2
#define CHART_COLOR_BLUE 3
#define CHART_COLOR_VERYRED 4
#define CHART_COLOR_ORANGE 5
#define CHART_COLOR_LIGHTBLUE 6

#define CHART_MAX_COLOR 7

gdImagePtr
chart_create_pie(gdImagePtr im, apr_array_header_t *data, char *graph_name, int width,
    int height);
gdImagePtr
chart_create_polyline(gdImagePtr im, apr_array_header_t *data, char *graph_name, int flags,
    int width, int height, int gamma);
gdImagePtr
chart_create_bars(gdImagePtr im, apr_array_header_t *data, char *graph_name, int flags,
    int width, int height, int gamma);


#endif /* CHART_H_ */
