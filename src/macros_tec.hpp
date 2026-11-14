#pragma once
// What the macro level activates: the expander, the structure modules and, unless
// TEXTOD_NO_CHARTS, the charts.
#include "expander_tec.hpp"
#include "datatable_tec.hpp"
#include "form_tec.hpp"
#include "graph_tec.hpp"
#include "sequence_tec.hpp"
#include "tree_tec.hpp"
#ifndef TEXTOD_NO_CHARTS
#include "charts/charts_tec.hpp"
#endif
