/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ke Yang, Tsinghua University 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <vector>
#include <utility>
#include <map>
#include <set>
#include <type_traits>

#include <gtest/gtest.h>

#include "storage.hpp"
#include "graph.hpp"
#include "walk.hpp"
#include "util.hpp"
#include "test.hpp"
#include "test_walk.hpp"
#include "../apps/deepwalk.hpp"

template<typename edge_data_t>
void test_deepwalk(vertex_id_t v_num, int worker_number)
{
    WalkEngine<edge_data_t, EmptyData> graph;
    graph.set_concurrency(worker_number);
    graph.load_graph(v_num, test_data_file);


    step_t walk_length = 60 + rand() % 20;
    MPI_Bcast(&walk_length, 1, get_mpi_data_type<step_t>(), 0, MPI_COMM_WORLD);
    walker_id_t walker_num = graph.get_vertex_num() * 50 + graph.get_edge_num() * 10 + rand() % 100;
    MPI_Bcast(&walker_num, 1, get_mpi_data_type<walker_id_t>(), 0, MPI_COMM_WORLD);

    deepwalk(&graph, walker_num, walk_length);
    std::vector<std::vector<vertex_id_t> > rw_sequences;
    graph.collect_walk_sequence(rw_sequences);

    if (get_mpi_rank() == 0)
    {
        Edge<edge_data_t> *std_edges;
        edge_id_t std_edge_num;
        read_graph(test_data_file, 0, 1, std_edges, std_edge_num);
        check_static_random_walk(v_num, std_edges, std_edge_num, rw_sequences);
    }
}

template<typename edge_data_t>
void test_deepwalk()
{
    size_t e_nums_arr[] = {1000, 2000, 4000, 5556, 8888, 10000, 20000, 100000};
    vertex_id_t v_num = 1000 + rand() % 500;
    std::vector<size_t> e_nums(e_nums_arr, e_nums_arr + 8);
    /*
    size_t e_nums_arr[] = {30};
    vertex_id_t v_num = 10;
    std::vector<size_t> e_nums(e_nums_arr, e_nums_arr + 1);
    */

    MPI_Bcast(&v_num, 1, get_mpi_data_type<vertex_id_t>(), 0, MPI_COMM_WORLD);

    for (auto &e_num : e_nums_arr)
    {
        if (get_mpi_rank() == 0)
        {
            gen_undirected_graph_file<edge_data_t>(v_num, e_num);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        int worker_number = rand() % 8 + 1;
        MPI_Bcast(&worker_number, 1, get_mpi_data_type<int>(), 0, MPI_COMM_WORLD);
        test_deepwalk<edge_data_t>(v_num, worker_number);
    }
    if (get_mpi_rank() == 0)
    {
        rm_test_graph_temp_file();
    }
}

TEST(DeepWalk, UnbiasedDeepWalk)
{
    test_deepwalk<EmptyData>();
}

TEST(DeepWalk, BiasedDeepWalk)
{
    test_deepwalk<real_t>();
}

GTEST_API_ int main(int argc, char *argv[])
{
    MPI_Instance mpi_instance(&argc, &argv);
    ::testing::InitGoogleTest(&argc, argv);
    mute_nonroot_gtest_events();
    int result = RUN_ALL_TESTS();
    return result;
}
