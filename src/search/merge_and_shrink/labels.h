#ifndef MERGE_AND_SHRINK_LABELS_H
#define MERGE_AND_SHRINK_LABELS_H

#include <memory>
#include <vector>

namespace merge_and_shrink {
class LabelsConstIterator {
    const std::vector<int> &label_costs;
    std::size_t current_index;

    void next_valid_index();
public:
    LabelsConstIterator(const std::vector<int> &label_costs, bool end);
    void operator++();

    int operator*() const {
        return int(current_index);
    }

    bool operator==(const LabelsConstIterator &rhs) const {
        return current_index == rhs.current_index;
    }

    bool operator!=(const LabelsConstIterator &rhs) const {
        return current_index != rhs.current_index;
    }
};

/*
  This class serves both as a container class to handle the set of all
  labels and to perform label reduction on this set.

  Labels are identified via integers indexing label_costs, which store their
  costs. When using label reductions, reduced labels are set to -1 in label
  costs.
*/
class Labels {
    std::vector<int> label_costs;
    int max_num_labels; // The maximum number of labels that can be created.
    int num_active_labels; // The current number of active (non-reduced) labels.
public:
    Labels(std::vector<int> &&label_costs, int max_num_labels);
    void reduce_labels(const std::vector<int> &old_labels);
    int get_label_cost(int label) const;
    void dump_labels() const;

    int get_size() const {
        return label_costs.size();
    }

    int get_max_num_labels() const {
        return max_num_labels;
    }

    int get_num_active_labels() const {
        return num_active_labels;
    }

    LabelsConstIterator begin() const {
        return LabelsConstIterator(label_costs, false);
    }

    LabelsConstIterator end() const {
        return LabelsConstIterator(label_costs, true);
    }
};
}

#endif
