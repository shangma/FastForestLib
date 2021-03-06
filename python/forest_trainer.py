from __future__ import division

from forest import Forest
from tree import ArrayTree
#from training_context import TrainingContext


class TrainingParameters(object):

    def __init__(self, num_of_trees=3, maximum_depth=20, num_of_features=100,
                 num_of_thresholds=50, minimum_information_gain=0.0, minimum_num_of_samples=100):
        self.numOfTrees = num_of_trees
        self.maximumDepth = maximum_depth
        self.numOfFeatures = num_of_features
        self.numOfThresholds = num_of_thresholds
        self.minimumInformationGain = minimum_information_gain
        self.minimumNumOfSamples = minimum_num_of_samples


class RandomForestTrainer(object):

    class _TrainingOperation:

        def __init__(self, sample_indices, weak_learner_context, training_parameters):
            # TODO: how to handle these assertions with extension types?
            # assert isinstance(training_context, TrainingContext)
            # assert isinstance(training_parameters, TrainingParameters)
            self._sample_indices = sample_indices
            self._weak_learner_context = weak_learner_context
            self._training_parameters = training_parameters

        def train_recursive(self, node, i_start, i_end, statistics=None, current_depth=1):
            prefix = current_depth * " "
            num_of_samples = i_end - i_start

            print("{}depth: {}, samples: {}".format(prefix, current_depth, num_of_samples))

            # define local aliases for some long variable names
            sample_indices = self._sample_indices[i_start:i_end]

            # assign statistics to node
            if statistics is None:
                statistics = self._weak_learner_context.compute_statistics(sample_indices)
            node.statistics = statistics

            # stop splitting the node if the minimum number of samples has been reached
            if num_of_samples < self._training_parameters.minimumNumOfSamples:
                node.leaf_node = True
                print("{}Minimum number of samples. Stopping".format(prefix))
                return

            # stop splitting the node if it is a leaf node
            if node.left_child is None:
                node.leaf_node = True
                print("{}Reached leaf node. Stopping.".format(prefix))
                return

            split_point_context = self._weak_learner_context.sample_split_points(
                sample_indices,
                self._training_parameters.numOfFeatures,
                self._training_parameters.numOfThresholds)

            # TODO: distribute features and thresholds to ranks > 0

            # compute the statistics for all feature and threshold combinations
            split_statistics = self._weak_learner_context.compute_split_statistics(sample_indices, split_point_context)

            # TODO: send statistics to rank 0
            # send split_statistics.get_buffer()

            # TODO: receive statistics from rank > 0
            # for received statistics
            #    split_statistics.accumulate(statistics)

            # find the best feature (only on rank 0)
            best_split_point_id, best_information_gain = self._weak_learner_context.select_best_split_point(
                node.statistics, split_statistics, return_information_gain=True)

            # TODO: send best feature, threshold and information gain to ranks > 0

            # TODO: move criterion into trainingContext?
            # stop splitting the node if the best information gain is below the minimum information gain
            if best_information_gain < self._training_parameters.minimumInformationGain:
                node.leaf_node = True
                print("{}Too little information gain. Stopping.".format(prefix))
                return

            # partition sample_indices according to the selected feature and threshold.
            # i.e. sample_indices[:i_split] will contain the left child indices
            # and sample_indices[i_split:] will contain the right child indices
            best_split_point = split_point_context.get_split_point(best_split_point_id)
            i_split = i_start + self._weak_learner_context.partition(sample_indices, best_split_point)

            node.split_point = best_split_point

            # TODO: can we reuse computed statistics from split_point_context???
            left_child_statistics = None
            right_child_statistics = None

            # train left and right child
            #print("{}Going left".format(prefix))
            self.train_recursive(node.left_child, i_start, i_split, left_child_statistics, current_depth + 1)
            #print("{}Going right".format(prefix))
            self.train_recursive(node.right_child, i_split, i_end, right_child_statistics, current_depth + 1)

    def train_forest(self, sample_indices, training_context, training_parameters):
        forest = Forest()
        for i in xrange(training_parameters.numOfTrees):
            # TODO: perform bagging on the samples
            tree = ArrayTree(training_parameters.maximumDepth)
            self.train_tree(tree, sample_indices, training_context, training_parameters)
            forest.append(tree)
        return forest

    def train_tree(self, tree, sample_indices, training_context, training_parameters):
        rf_operation = self._TrainingOperation(sample_indices, training_context, training_parameters)
        i_start = 0
        i_end = len(sample_indices)
        print("Training tree")
        rf_operation.train_recursive(tree.root, i_start, i_end)
        return tree
