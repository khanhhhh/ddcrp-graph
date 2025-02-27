//
// Created by khanh on 13/10/20.
//

#ifndef FYP_ASSIGNMENT_H
#define FYP_ASSIGNMENT_H

#include <set>
#include "common.h"

using Customer = uint64;
using Table = uint64;

const Customer customer_nil = uint64_nil;
const Table table_nil = uint64_nil;

class Assignment {
public:
    explicit Assignment(Customer num_customers);

    ~Assignment() = default;

    void set_loglikelihood_func(std::function<float64(const std::vector<Customer>&)> loglikelihood_func);

    void unlink(Customer source);

    void link(Customer source, Customer target);

    [[nodiscard]] Customer num_customers() const;

    [[nodiscard]] Table num_tables() const;

    [[nodiscard]] Table table(Customer customer) const;

    [[nodiscard]] std::vector<Customer> component(Customer customer) const;

    [[nodiscard]] std::vector<Table> customer2table() const;

    float64 loglikelihood(Customer source) const;


private:
    struct Node {
        Customer m_parent;
        std::set<Customer> m_children;

        Node();

        ~Node() = default;
    };

    uint64 m_num_customers;
    std::vector<Node> m_adjacency_list;
    std::vector<Table> m_customer2table;
    std::map<Table, std::set<Customer>> m_table2customer;
    std::map<Table, float64> m_table2loglikelihood;
    uint64 m_table_count;
    std::function<float64(const std::vector<Customer> &)> m_loglikelihood_func; // loglikelihood of a compoentn

    std::set<Customer> weakly_connected_component(Customer customer);

};

Assignment::Node::Node() :
        m_parent(customer_nil), m_children() {}

Assignment::Assignment(Customer num_customers) :
//init default, each customer sits in one table
//:param num_customers: number of customers
        m_num_customers(num_customers),
        m_adjacency_list(),
        m_customer2table(),
        m_table2customer(),
        m_table2loglikelihood(),
        m_table_count(num_customers),
        m_loglikelihood_func([](const std::vector<Customer>&) -> float64 {
            return 0;
        })
        {
    for (Customer customer = 0; customer < num_customers; customer++) {
        m_adjacency_list.emplace_back();
        m_customer2table.push_back(customer);
        m_table2customer.emplace(customer, std::set<Customer>({customer}));
        m_table2loglikelihood.emplace(customer, 0);
    }
}

std::set<Customer> Assignment::weakly_connected_component(Customer customer) {
    std::set<Customer> visited;
    std::vector<Customer> frontier({customer});

    auto is_in_list = [](const std::vector<Customer> &list, Customer customer) -> bool {
        for (Customer item: list) {
            if (item == customer) {
                return true;
            }
        }
        return false;
    };
    while (not frontier.empty()) {
        auto current = frontier.back();
        frontier.pop_back();
        visited.insert(current);
        auto node = m_adjacency_list[current];
        auto adding = node.m_children;
        adding.insert(node.m_parent);
        for (auto new_customer: adding) {
            if (new_customer != customer_nil and not is_in_list(frontier, new_customer) and
                not(visited.find(new_customer) != visited.end())) {
                frontier.push_back(new_customer);
            }
        };
    }
    return visited;
}

void Assignment::unlink(Customer source) {
    auto target = m_adjacency_list[source].m_parent;
    if (target == customer_nil) {
        return;
    }
    // remove link
    m_adjacency_list[target].m_children.erase(source);
    m_adjacency_list[source].m_parent = customer_nil;
    auto new_source_component = weakly_connected_component(source);
    if (new_source_component.find(target) != new_source_component.end()) {
        return;
    }
    // split
    // update assingment
    auto new_source_table = m_table_count;
    m_table_count++;
    for (auto new_source_customer: new_source_component) {
        m_customer2table[new_source_customer] = new_source_table;
    }
    // update table
    auto new_target_table = m_customer2table[target];
    auto& new_target_component = m_table2customer[new_target_table];
    for (auto new_source_customer: new_source_component) {
        new_target_component.erase(new_source_customer);
    }
    m_table2customer.emplace(new_source_table, new_source_component);
    // update loglikelihood
    m_table2loglikelihood[new_target_table] = m_loglikelihood_func(std::vector<Customer>(
            m_table2customer[new_target_table].begin(), m_table2customer[new_target_table].end()
    ));
    m_table2loglikelihood.emplace(new_source_table, m_loglikelihood_func(std::vector<Customer>(
            m_table2customer[new_source_table].begin(), m_table2customer[new_source_table].end()
    )));
}

void Assignment::link(Customer source, Customer target) {
    // add link
    m_adjacency_list[source].m_parent = target;
    m_adjacency_list[target].m_children.insert(source);
    if (m_customer2table[source] == m_customer2table[target]) {
        return;
    }
    // join
    // update assingment
    auto new_join_table = m_customer2table[source];
    auto old_target_table = m_customer2table[target];
    auto& old_target_component = m_table2customer[old_target_table];
    for (auto old_target_customer: old_target_component) {
        m_customer2table[old_target_customer] = new_join_table;
    }
    // update table
    auto& new_join_component = m_table2customer[new_join_table];
    new_join_component.insert(old_target_component.begin(), old_target_component.end());
    m_table2customer.erase(old_target_table);
    // update loglikelihood
    m_table2loglikelihood[new_join_table] = m_loglikelihood_func(std::vector<Customer>(
            m_table2customer[new_join_table].begin(), m_table2customer[new_join_table].end()
            ));
    m_table2loglikelihood.erase(old_target_table);
}

Customer Assignment::num_customers() const {
    return m_num_customers;
}

Customer Assignment::num_tables() const {
    return m_table2customer.size();
}

Table Assignment::table(Customer customer) const {
    return m_customer2table[customer];
}

std::vector<Customer> Assignment::component(Customer customer) const {
    auto component = m_table2customer.find(m_customer2table[customer])->second;
    return std::vector<Customer>(component.begin(), component.end());
}

std::vector<Table> Assignment::customer2table() const {
    return std::vector<Table>(m_customer2table);
}

void Assignment::set_loglikelihood_func(std::function<float64(const std::vector<Customer>&)> loglikelihood_func) {
    m_loglikelihood_func = loglikelihood_func;
    for (auto it = m_table2customer.begin(); it != m_table2customer.end(); it++) {
        Table table = it->first;
        auto component = it->second;
        m_table2loglikelihood[table] = m_loglikelihood_func(std::vector<Customer>(component.begin(), component.end()));
    }
}

float64 Assignment::loglikelihood(Customer source) const {
    return m_table2loglikelihood.find(m_customer2table[source])->second;
}


#endif //FYP_ASSIGNMENT_H
