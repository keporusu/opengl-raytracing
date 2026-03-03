#pragma once
#include "../BufferSizeSettings.hpp"
#include "AABB.hpp"
#include "../Primitives/Primitives.hpp"
#include <vector>
#include <random>

struct SubUBO_BVH
{
    AlignedBox aabb;
    int left = -1;
    int right = -1;
    int prim_index = -1;
    float _padding[3];
};
struct UBO_BVH
{
    int node_count;
    float _padding[3];
    SubUBO_BVH nodes[MAX_BVH_NODES];
};

class BVHNode
{
public:
    BVHNode() = default;
    BVHNode(std::vector<std::shared_ptr<Primitive>> &primitives, size_t start, size_t end)
    {
        std::random_device seed_gen;
        std::mt19937 engine(seed_gen());
        std::uniform_int_distribution<int> dist(0, 2);

        // 軸を選択
        int axis = dist(engine);

        // ソートのルールを選択
        auto box_compare = (axis == 0)   ? box_compare_x
                           : (axis == 1) ? box_compare_y
                                         : box_compare_z;

        size_t object_span = end - start;
        if (object_span == 1)
        {
            // 葉の作成
            primitive_index = (int)start;
            // プリミティブからAABBを作成
            aabb = AlignedBox(primitives[primitive_index]->GetAABB(), primitives[primitive_index]->GetAABB());
        }
        else
        {
            // 選択したルールでソート
            std::sort(primitives.begin() + start, primitives.begin() + end, box_compare);
            int mid = start + object_span / 2;
            left = std::make_unique<BVHNode>(primitives, start, mid);
            right = std::make_unique<BVHNode>(primitives, mid, end);
            // AABBから更にAABBを作成
            aabb = AlignedBox(left->GetAABB(), right->GetAABB());
        }
    }

    // ソートのルール（軸を選択して、minが小さい順に並べる）
    static bool box_compare_x(const std::shared_ptr<Primitive> a, const std::shared_ptr<Primitive> b)
    {
        const auto a_aabb = a->GetAABB();
        const auto b_aabb = b->GetAABB();
        return a_aabb.x_min < b_aabb.x_min;
    }
    static bool box_compare_y(const std::shared_ptr<Primitive> a, const std::shared_ptr<Primitive> b)
    {
        const auto a_aabb = a->GetAABB();
        const auto b_aabb = b->GetAABB();
        return a_aabb.y_min < b_aabb.y_min;
    }
    static bool box_compare_z(const std::shared_ptr<Primitive> a, const std::shared_ptr<Primitive> b)
    {
        const auto a_aabb = a->GetAABB();
        const auto b_aabb = b->GetAABB();
        return a_aabb.z_min < b_aabb.z_min;
    }

    AlignedBox GetAABB() const { return aabb; }

    std::vector<SubUBO_BVH> FlattenAndGetUBO()
    {
        std::vector<SubUBO_BVH> bvh_ubo;
        flatten(*this, bvh_ubo);
        return bvh_ubo;
    }

private:
    // 左ノード,右ノード（葉には存在しない）
    std::unique_ptr<BVHNode> left, right;
    AlignedBox aabb;

    // このaabbが囲っているプリミティブ（葉にのみ存在）
    int primitive_index = -1;

    static int flatten(const BVHNode &node, std::vector<SubUBO_BVH> &bvh_ubo)
    {
        // インデックス番号を取得し、予約する
        int this_index = (int)bvh_ubo.size();
        bvh_ubo.push_back({});

        SubUBO_BVH ubo_element;
        ubo_element.aabb = node.aabb;
        ubo_element.prim_index = node.primitive_index;
        ubo_element.left = node.left ? flatten(*node.left, bvh_ubo) : -1;
        ubo_element.right = node.right ? flatten(*node.right, bvh_ubo) : -1;

        bvh_ubo[this_index] = ubo_element;
        return this_index;
    }
};