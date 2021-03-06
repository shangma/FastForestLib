//
//  level_forest_trainer.h
//  DistRandomForest
//
//  Created by Benjamin Hepp on 30/09/15.
//
//

#pragma once

#include <iostream>
#include <sstream>
#include <map>

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/utility/enable_if.hpp>

#include "ait.h"
#include "iterator_utils.h"
#include "training.h"
#include "forest.h"
#include "weak_learner.h"
#include "common.h"

namespace ait
{

template <template <typename> class TWeakLearner, typename TSampleIterator>
class LevelForestTrainer
{
public:
    using ParametersT = LevelTrainingParameters;

    using SampleIteratorT = TSampleIterator;
    using SampleT = typename TSampleIterator::value_type;
    using SamplePointerT = const SampleT*;
    using SamplePointerIteratorT = typename std::vector<SamplePointerT>::const_iterator;
    using SamplePointerIteratorWrapperT = ait::PointerIteratorWrapper<SamplePointerIteratorT, const SampleT>;
    
    using WeakLearnerT = TWeakLearner<SamplePointerIteratorWrapperT>;
    using RandomEngineT = typename WeakLearnerT::RandomEngineT;

    using StatisticsT = typename WeakLearnerT::StatisticsT;
    using SplitPointT = typename WeakLearnerT::SplitPointT;
    using SplitPointCandidatesT = typename WeakLearnerT::SplitPointCandidatesT;
    using ForestT = Forest<SplitPointT, StatisticsT>;
    using TreeT = Tree<SplitPointT, StatisticsT>;
    using NodeType = typename TreeT::NodeT;
    using NodeIterator = typename TreeT::NodeIterator;


protected:
    const WeakLearnerT weak_learner_;
    const ParametersT training_parameters_;
    
    void output_spaces(std::ostream& stream, int num_of_spaces) const
    {
        for (int i = 0; i < num_of_spaces; i++)
            stream << " ";
    }
    
    struct SplitInformation
    {
        scalar_type information_gain;
        size_type total_num_of_samples;
        size_type left_num_of_samples;
        size_type right_num_of_samples;
    };
    
    template <typename T>
    class TreeNodeMap
    {
        using MapT = std::map<size_type, T>;
        TreeT* tree_;
        MapT map_;

        template <typename TBaseIterator, typename TValue>
        class iterator_ : public boost::iterator_adaptor<iterator_<TBaseIterator, TValue>, TBaseIterator, TValue>
        {
            using BaseT = boost::iterator_adaptor<iterator_<TBaseIterator, TValue>, TBaseIterator, TValue>;
            TreeT* tree_;

        public:
            explicit iterator_(TreeT* tree, TBaseIterator it)
            : iterator_::iterator_adaptor_(it), tree_(tree)
            {}

            template <typename TOtherBaseIterator, typename TOtherValue>
            iterator_(
                      const iterator_<TBaseIterator, TOtherValue>& other,
                      typename boost::enable_if<
                      boost::is_convertible<TBaseIterator, TOtherBaseIterator>, int>::type = 0
                      )
            : iterator_::iterator_adaptor_(other.base()), tree_(other.tree_)
            {}
            
            typename TreeT::NodeIterator node_iterator()
            {
                return tree_->get_node_iterator(this->base()->first);
            }
            
            typename TreeT::ConstNodeIterator node_iterator() const
            {
                return tree_->get_node_iterator(this->base()->first);
            }

            typename TreeT::NodeEntry& node_entry()
            {
                return tree_->get_node(this->base()->first);
            }
            
            const typename TreeT::NodeEntry& node_entry() const
            {
                return tree_->get_node(this->base()->first);
            }
            
            typename TreeT::NodeT& node()
            {
                return node_entry().node;
            }
            
            const typename TreeT::NodeT& node() const
            {
                return node_entry().node;
            }

        private:
			using IteratorAdaptorType = boost::iterator_adaptor<iterator_<TBaseIterator, TValue>, TBaseIterator, TValue>;

            friend class boost::iterator_core_access;
            template <typename, typename> friend class iterator_;

            
            typename IteratorAdaptorType::iterator_facade_::reference dereference() const
            {
                return this->base()->second;
            }
        };

    public:
        using iterator = iterator_<typename MapT::iterator, T>;
        using const_iterator = iterator_<typename MapT::const_iterator, const T>;

        explicit TreeNodeMap(TreeT& tree)
        : tree_(&tree)
        {}
        
        explicit TreeNodeMap(TreeT* tree)
        : tree_(tree)
        {}

        TreeNodeMap(const TreeNodeMap& map)
        : tree_(map.tree_), map_(map.map_)
        {}

        TreeNodeMap(TreeNodeMap&& map) noexcept
        : tree_(map.tree_), map_(std::move(map.map_))
        {}

        TreeNodeMap& operator=(const TreeNodeMap& other)
        {
            if (this != &other)
            {
                tree_ = other.tree_;
                map_ = other.map_;
            }
            return *this;
        }
        
        TreeNodeMap& operator=(TreeNodeMap& other)
        {
            if (this != &other)
            {
                tree_ = other.tree_;
                map_ = other.map_;
            }
            return *this;
        }

        TreeNodeMap& operator=(TreeNodeMap&& other)
        {
            if (this != &other)
            {
                tree_ = other.tree_;
                map_ = std::move(other.map_);
            }
            return *this;
        }

        MapT& base_map()
        {
            return map_;
        }

        size_type size() const
        {
            return map_.size();
        }

        void clear()
        {
            map_.clear();
        }

        iterator begin()
        {
            return iterator(tree_, map_.begin());
        }
        
        iterator end()
        {
            return iterator(tree_, map_.end());
        }

        const_iterator cbegin() const
        {
            return const_iterator(tree_, map_.cbegin());
        }
        
        const_iterator cend() const
        {
            return const_iterator(tree_, map_.cend());
        }
        
        T& operator[](const size_type& index)
        {
            return map_[index];
        }

        T& operator[](const typename TreeT::ConstNodeIterator& it)
        {
            return map_[it.get_node_index()];
        }

        T& at(const size_type& index)
        {
            return map_.at(index);
        }
        
        const T& at(const size_type& index) const
        {
            return map_.at(index);
        }

        T& at(const typename TreeT::ConstNodeIterator& it)
        {
            return map_.at(it.get_node_index());
        }
        
        const T& at(const typename TreeT::ConstNodeIterator& it) const
        {
            return map_.at(it.get_node_index());
        }
        
        iterator find(const typename TreeT::ConstNodeIterator& it)
        {
            typename MapT::iterator map_it = map_.find(it.get_node_index());
            return iterator(tree_, map_it);
        }

        const_iterator find(const typename TreeT::ConstNodeIterator& it) const
        {
            typename MapT::const_iterator map_it = map_.find(it.get_node_index());
            return const_iterator(tree_, map_it);
        }

    private:
#ifdef SERIALIZE_WITH_BOOST
        friend class boost::serialization::access;
        
        template <typename Archive>
        void serialize(Archive& archive, const unsigned int version, typename enable_if_boost_archive<Archive>::type* = nullptr)
        {
#if AIT_PROFILE || AIT_PROFILE_DISTRIBUTED
            auto start_time = std::chrono::high_resolution_clock::now();
            log_profile(false) << "Serializing tree node map with boost ...";
#endif
            archive & map_;
#if AIT_PROFILE || AIT_PROFILE_DISTRIBUTED
            ait::log_profile() << "Finished in " << compute_elapsed_milliseconds(start_time) << " ms";
#endif
        }
#endif

        friend class cereal::access;
        
        template <typename Archive>
        void serialize(Archive& archive, const unsigned int version, typename disable_if_boost_archive<Archive>::type* = nullptr)
        {
#if AIT_PROFILE || AIT_PROFILE_DISTRIBUTED
            auto start_time = std::chrono::high_resolution_clock::now();
            log_profile(false) << "Serializing tree node map with Cereal ...";
#endif
            archive(cereal::make_nvp("map", map_));
#if AIT_PROFILE || AIT_PROFILE_DISTRIBUTED
            ait::log_profile() << "Finished in " << compute_elapsed_milliseconds(start_time) << " ms";
#endif
        }

    };

    virtual TreeNodeMap<SplitPointCandidatesT> sample_split_points_batch(TreeT& tree, const TreeNodeMap<std::vector<SamplePointerT>>& node_to_sample_map, RandomEngineT& rnd_engine) const
    {
        TreeNodeMap<SplitPointCandidatesT> split_points_batch(tree);
        for (auto map_it = node_to_sample_map.cbegin(); map_it != node_to_sample_map.cend(); ++map_it)
        {
            typename WeakLearnerT::SampleIteratorT sample_it_begin = make_pointer_iterator_wrapper(map_it->cbegin());
            typename WeakLearnerT::SampleIteratorT sample_it_end = make_pointer_iterator_wrapper(map_it->cend());
            split_points_batch[map_it.node_iterator()] = weak_learner_.sample_split_points(sample_it_begin, sample_it_end, rnd_engine);
        }
        // TODO: Move semantics?
        return split_points_batch;
    }
    
    virtual TreeNodeMap<SplitStatistics<StatisticsT>> compute_split_statistics_batch(TreeT& tree, const TreeNodeMap<std::vector<SamplePointerT>>& node_to_sample_map, const TreeNodeMap<SplitPointCandidatesT>& split_points_batch) const
    {
        TreeNodeMap<SplitStatistics<StatisticsT>> split_statistics_batch(tree);
        for (auto map_it = node_to_sample_map.cbegin(); map_it != node_to_sample_map.cend(); ++map_it)
        {
            const SplitPointCandidatesT& split_points = split_points_batch.at(map_it.node_iterator());
            typename WeakLearnerT::SampleIteratorT sample_it_begin = make_pointer_iterator_wrapper(map_it->cbegin());
            typename WeakLearnerT::SampleIteratorT sample_it_end = make_pointer_iterator_wrapper(map_it->cend());
            // TODO: Move semantics?
#if AIT_MULTI_THREADING
            if (training_parameters_.num_of_threads == 1)
            {
                split_statistics_batch[map_it.node_iterator()] = weak_learner_.compute_split_statistics(sample_it_begin, sample_it_end, split_points);
            }
            else
            {
            	split_statistics_batch[map_it.node_iterator()] = weak_learner_.compute_split_statistics_parallel(sample_it_begin, sample_it_end, split_points, training_parameters_.num_of_threads);
            }
#else
            split_statistics_batch[map_it.node_iterator()] = weak_learner_.compute_split_statistics(sample_it_begin, sample_it_end, split_points);
#endif
        }
        return split_statistics_batch;
    }
    
    virtual TreeNodeMap<std::tuple<SplitPointT, SplitInformation>> find_best_split_point_batch(TreeT& tree, const TreeNodeMap<SplitPointCandidatesT>& split_points_batch, const TreeNodeMap<StatisticsT>& current_statistics, const TreeNodeMap<SplitStatistics<StatisticsT>>& split_statistics_batch) const
    {
        TreeNodeMap<std::tuple<SplitPointT, SplitInformation>> best_split_point_batch(tree);
        for (auto map_it = split_statistics_batch.cbegin(); map_it != split_statistics_batch.cend(); ++map_it)
        {
            const SplitPointCandidatesT& split_points = split_points_batch.at(map_it.node_iterator());
            std::tuple<size_type, scalar_type> best_split_point_tuple = weak_learner_.find_best_split_point_tuple(current_statistics.at(map_it.node_iterator()), *map_it);
            size_type best_split_point_index = std::get<0>(best_split_point_tuple);
            SplitPointT best_split_point = split_points.get_split_point(best_split_point_index);
            SplitInformation best_split_information;
            best_split_information.information_gain = std::get<1>(best_split_point_tuple);
            best_split_information.total_num_of_samples = current_statistics.at(map_it.node_iterator()).num_of_samples();
            best_split_information.left_num_of_samples = map_it->get_left_statistics(best_split_point_index).num_of_samples();
            best_split_information.right_num_of_samples = map_it->get_right_statistics(best_split_point_index).num_of_samples();
            best_split_point_batch[map_it.node_iterator()] = std::make_tuple(best_split_point, best_split_information);
        }
        return best_split_point_batch;
    }

    virtual TreeNodeMap<StatisticsT> compute_statistics_batch(TreeT& tree, TreeNodeMap<std::vector<SamplePointerT>>& node_to_sample_map) const
    {
        TreeNodeMap<StatisticsT> statistics_batch(tree);
        for (auto map_it = node_to_sample_map.cbegin(); map_it != node_to_sample_map.cend(); ++map_it)
        {
            StatisticsT statistics = weak_learner_.create_statistics();
            typename WeakLearnerT::SampleIteratorT sample_it_begin = make_pointer_iterator_wrapper(map_it->cbegin());
            typename WeakLearnerT::SampleIteratorT sample_it_end = make_pointer_iterator_wrapper(map_it->cend());
            statistics.accumulate(sample_it_begin, sample_it_end);
            statistics_batch[map_it.node_iterator()] = std::move(statistics);
        }
        return statistics_batch;
    }
    
    virtual void update_node_statistics_batch(const TreeNodeMap<StatisticsT>& statistics_batch) const
    {
        for (auto map_it = statistics_batch.cbegin(); map_it != statistics_batch.cend(); ++map_it)
        {
            map_it.node().set_statistics(*map_it);
        }
    }

    TreeNodeMap<std::vector<SamplePointerT>> get_sample_node_map(
                                                                 TreeT& tree,
                                                                 typename TreeT::ConstNodeIterator node_iter_start,
                                                                 typename TreeT::ConstNodeIterator node_iter_end,
                                                                 TSampleIterator samples_start,
                                                                 TSampleIterator samples_end
                                                                 ) const
    {
        TreeNodeMap<std::vector<SamplePointerT>> node_to_sample_map(tree);
        for (auto node_it = node_iter_start; node_it != node_iter_end; ++node_it)
        {
            // Make sure every node of the tree-level has an entry in the map
            node_to_sample_map[node_it];
        }
        for (auto it = samples_start; it != samples_end; ++it)
        {
            const typename TreeT::NodeIterator node_it = tree.evaluate(*it);
            // Some nodes might already be children of leaf-nodes so sample evaluating will terminate before
            if (node_to_sample_map.find(node_it) != node_to_sample_map.end())
            {
                SamplePointerT sample_ptr = &(*it);
                node_to_sample_map[node_it].push_back(sample_ptr);
            }
        }
        return node_to_sample_map;
    }
    
    TreeNodeMap<std::vector<SamplePointerT>> get_sample_node_map_part(
                                                                 TreeT& tree,
                                                                 TreeNodeMap<std::vector<SamplePointerT>>& node_to_sample_map,
                                                                 typename TreeT::ConstNodeIterator node_iter_start,
                                                                 typename TreeT::ConstNodeIterator node_iter_end
                                                                 ) const
    {
        TreeNodeMap<std::vector<SamplePointerT>> node_to_sample_map_part(tree);
        for (auto node_it = node_iter_start; node_it != node_iter_end; ++node_it)
        {
            node_to_sample_map_part[node_it] = node_to_sample_map[node_it];
        }
        return node_to_sample_map_part;
    }

    TreeNodeMap<std::vector<SamplePointerT>> get_sample_node_map(TreeT& tree, typename TreeT::TreeLevel& tl, TSampleIterator samples_start, TSampleIterator samples_end) const
    {
        TreeNodeMap<std::vector<SamplePointerT>> node_to_sample_map(tree);
        for (auto node_it = tl.begin(); node_it != tl.end(); ++node_it)
        {
            // Make sure every node of the tree-level has an entry in the map
            node_to_sample_map[node_it];
        }
        for (auto it = samples_start; it != samples_end; ++it)
        {
            const typename TreeT::NodeIterator node_it = tree.evaluate(*it);
            // Some nodes might already be children of leaf-nodes so sample evaluating will terminate before
            if (node_it >= tl.begin())
            {
                SamplePointerT sample_ptr = &(*it);
                node_to_sample_map[node_it].push_back(sample_ptr);
            }
        }
        return node_to_sample_map;
    }

public:
    explicit LevelForestTrainer(const WeakLearnerT& weak_learner, const ParametersT& training_parameters)
    : weak_learner_(weak_learner), training_parameters_(training_parameters)
    {}
    
    virtual ~LevelForestTrainer()
    {}
    
    const ParametersT& get_parameters() const
    {
        return training_parameters_;
    }
    
    virtual void train_tree_level_part(
    		TreeNodeMap<std::vector<SamplePointerT>> node_to_sample_map,
    		TreeT& tree,
			size_type current_level,
			typename TreeT::ConstNodeIterator node_iter_start,
			typename TreeT::ConstNodeIterator node_iter_end,
			SampleIteratorT samples_start,
			SampleIteratorT samples_end,
			RandomEngineT& rnd_engine) const
    {
        const TreeNodeMap<StatisticsT>& current_statistics = compute_statistics_batch(tree, node_to_sample_map);
        update_node_statistics_batch(current_statistics);
        if (current_level < training_parameters_.tree_depth)
        {
            TreeNodeMap<SplitPointCandidatesT> split_points_batch = sample_split_points_batch(tree, node_to_sample_map, rnd_engine);
            TreeNodeMap<SplitStatistics<StatisticsT>> split_statistics_batch = compute_split_statistics_batch(tree, node_to_sample_map, split_points_batch);
            TreeNodeMap<std::tuple<SplitPointT, SplitInformation>> best_split_point_batch = find_best_split_point_batch(tree, split_points_batch, current_statistics, split_statistics_batch);
            for (auto map_it = best_split_point_batch.begin(); map_it != best_split_point_batch.end(); ++map_it)
            {
                typename TreeT::NodeIterator node_it = map_it.node_iterator();
                node_it->set_split_point(std::get<0>(*map_it));
                SplitInformation& split_information = std::get<1>(*map_it);
                node_it.left_child().set_leaf(true);
                node_it.right_child().set_leaf(true);
                if (split_information.information_gain < training_parameters_.minimum_information_gain
                    || split_information.total_num_of_samples < training_parameters_.minimum_num_of_samples)
                {
                    node_it.set_leaf(true);
                }
                else
                {
                    node_it.set_leaf(false);
                }
            }
        }
    }

    virtual void train_tree_level(TreeT& tree, size_type current_level, SampleIteratorT samples_start, SampleIteratorT samples_end, RandomEngineT& rnd_engine) const
    {
        typename TreeT::TreeLevel tl(tree, current_level);
        TreeNodeMap<std::vector<SamplePointerT>> node_to_sample_map = get_sample_node_map(tree, tl.cbegin(), tl.cend(), samples_start, samples_end);
        size_type part = 0;
        for (auto node_it = tl.cbegin(); node_it < tl.cend(); )
        {
            auto node_it_next = node_it;
            if (training_parameters_.level_part_size > 0)
            {
                node_it_next += training_parameters_.level_part_size;
                if (node_it_next > tl.cend())
                {
                    node_it_next = tl.cend();
                }
            }
            else
            {
                node_it_next = tl.cend();
            }
            log_info() << "  Part " << part << ", # nodes: " << (node_it_next - node_it);
            ++part;
            TreeNodeMap<std::vector<SamplePointerT>> node_to_sample_map_part = get_sample_node_map_part(tree, node_to_sample_map, node_it, node_it_next);
            train_tree_level_part(node_to_sample_map_part, tree, current_level, node_it, node_it_next, samples_start, samples_end, rnd_engine);
            node_it = node_it_next;
        }
    }

    TreeT train_tree(SampleIteratorT samples_start, SampleIteratorT samples_end, RandomEngineT& rnd_engine) const
    {
        TreeT tree(training_parameters_.tree_depth);
        tree.get_root_iterator().set_leaf();
        log_info() << "Training tree, # samples: " << (samples_end - samples_start);
        for (size_type current_level = 1; current_level <= training_parameters_.tree_depth; current_level++)
        {
            typename TreeT::TreeLevel tl(tree, current_level);
            log_info() << "Training level " << current_level
                << ", # nodes: " << (tl.cend() - tl.cbegin())
                << ", # samples: " << (samples_end - samples_start);
#if AIT_PROFILE
            auto start_time = std::chrono::high_resolution_clock::now();
#endif
            train_tree_level(tree, current_level, samples_start, samples_end, rnd_engine);
#if AIT_PROFILE
            log_profile() << "Training level " << current_level << " took " << compute_elapsed_seconds(start_time) << " s";
#endif

            ait::log_info() << "Checkpoint. Saving temporary forest";
            // Optionally: Serialize temporary tree to JSON file.
            if (!training_parameters_.temporary_json_tree_file_prefix.empty())
            {
                std::ostringstream ostr;
                ostr << training_parameters_.temporary_json_tree_file_prefix << "_" << current_level;
                std::string tree_filename = ostr.str();
                ait::write_tree_to_json_file(tree_filename, tree);
            }
            // Optionally: Serialize temporary tree to binary file.
            if (!training_parameters_.temporary_json_tree_file_prefix.empty())
            {
                std::ostringstream ostr;
                ostr << training_parameters_.temporary_json_tree_file_prefix << "_" << current_level;
                std::string tree_filename = ostr.str();
                ait::write_tree_to_binary_file(tree_filename, tree);
            }
        }
        return tree;
    }
    
    TreeT train_tree(SampleIteratorT samples_start, SampleIteratorT samples_end) const
    {
        RandomEngineT rnd_engine;
        return train_tree(samples_start, samples_end, rnd_engine);
    }

    ForestT train_forest(SampleIteratorT samples_start, SampleIteratorT samples_end, RandomEngineT& rnd_engine) const
    {
        ForestT forest;
        for (size_type i=0; i < training_parameters_.num_of_trees; i++)
        {
            TreeT tree = train_tree(samples_start, samples_end, rnd_engine);
            forest.add_tree(std::move(tree));

            // Optionally: Serialize temporary forest to JSON file.
            if (!training_parameters_.temporary_json_forest_file_prefix.empty())
            {
                std::ostringstream ostr;
                ostr << training_parameters_.temporary_json_forest_file_prefix << "_" << i;
                std::string forest_file = ostr.str();
                ait::log_info(false) << "Writing temporary json forest file " << forest_file << "... " << std::flush;
                std::ofstream ofile(forest_file);
                cereal::JSONOutputArchive oarchive(ofile);
                oarchive(cereal::make_nvp("forest", forest));
                ait::log_info(false) << " Done." << std::endl;
            }
            // Optionally: Serialize temporary forest to binary file.
            if (!training_parameters_.temporary_binary_forest_file_prefix.empty())
            {
                std::ostringstream ostr;
                ostr << training_parameters_.temporary_json_forest_file_prefix << "_" << i;
                std::string forest_file = ostr.str();
                ait::log_info(false) << "Writing temporary binary forest file " << forest_file << "... " << std::flush;
                std::ofstream ofile(forest_file, std::ios_base::binary);
                cereal::BinaryOutputArchive oarchive(ofile);
                oarchive(cereal::make_nvp("forest", forest));
                ait::log_info(false) << " Done." << std::endl;
            }
        }
        return forest;
    }

    ForestT train_forest(SampleIteratorT samples_start, SampleIteratorT samples_end) const
    {
        RandomEngineT rnd_engine;
        return train_forest(samples_start, samples_end, rnd_engine);
    }

};

}

namespace boost {
namespace serialization {

// TODO: Why is this not working???
template<typename Archive, template <typename> class TWeakLearner, typename TSampleIterator, typename T>
inline void save_construct_data(Archive& ar, const typename ait::LevelForestTrainer<TWeakLearner, TSampleIterator>::template TreeNodeMapWithIndex<T>* obj, const unsigned int file_version)
{
    ait::log_info() << "Saving TreeNodeMapWithIndex";
    ar << obj->tree_;
}

template<typename Archive, template <typename> class TWeakLearner, typename TSampleIterator, typename T>
inline void load_construct_data(Archive& ar, typename ait::LevelForestTrainer<TWeakLearner, TSampleIterator>::template TreeNodeMapWithIndex<T>* obj, const unsigned int file_version)
{
    ait::log_info() << "Reconstructing TreeNodeMapWithIndex";
    typename ait::LevelForestTrainer<TWeakLearner, TSampleIterator>::TreeT* tree;
    ar >> tree;
    ::new(obj) typename ait::LevelForestTrainer<TWeakLearner, TSampleIterator>::template TreeNodeMapWithIndex<T>(tree);
}

}
}
