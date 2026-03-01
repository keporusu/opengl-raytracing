#pragma once
#include "AABB.hpp"
#include "../Primitives/Primitives.hpp"
#include <vector>
#include <random>

class BVHNode
{
public:
    BVHNode(std::vector<Primitive> &primitives, size_t start, size_t end)
    {
        std::random_device seed_gen;
        std::mt19937 engine(seed_gen());
        std::uniform_int_distribution<float> dist(0, 2);

        // 軸を選択
        int axis = dist(engine);

        size_t object_span = end - start;
        if (object_span == 1)
        {
            // 葉の作成
            primitive_index = (int)start;
            // プリミティブからAABBを作成
            aabb = AlignedBox(primitives[primitive_index].GetAABB(), primitives[primitive_index].GetAABB());
        }
        else
        {
            //選択したルールでソート
            std::sort(primitives.begin() + start, primitives.begin() + end, box_compare);
            int mid = start + object_span / 2;
            left = std::make_shared<BVHNode>(primitives, start, mid);
            right = std::make_shared<BVHNode>(primitives, mid, end);
            // AABBから更にAABBを作成
            aabb = AlignedBox(left->GetAABB(), right->GetAABB());
        }
    }

    //ソートのルール（軸を選択して、minが小さい順に並べる）
    static bool box_compare(const Primitive &a, const Primitive &b, int axis)
    {
        const auto a_aabb = a.GetAABB();
        const auto b_aabb = b.GetAABB();
        if (axis == 0)
        {
            return a_aabb.x_min < b_aabb.x_min;
        }
        else if (axis == 1)
        {
            return a_aabb.y_min < b_aabb.y_min;
        }
        else if (axis == 2)
        {
            return a_aabb.z_min < b_aabb.z_min;
        }
        else
        {
            throw std::logic_error("存在しない軸が選択されています");
        }
    }

    AlignedBox GetAABB() const { return aabb; }

private:
    // 左ノード,右ノード（葉には存在しない）
    std::shared_ptr<BVHNode> left, right;
    AlignedBox aabb;

    // このaabbが囲っているプリミティブ（葉にのみ存在）
    int primitive_index = -1;
};