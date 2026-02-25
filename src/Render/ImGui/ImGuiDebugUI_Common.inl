namespace rendern::ui
{
    static bool DragVec3(const char* label, mathUtils::Vec3& v, float speed = 0.05f, float minv = 0.0f, float maxv = 0.0f)
    {
        float a[3] = { v.x, v.y, v.z };
        const bool changed = ImGui::DragFloat3(label, a, speed, minv, maxv, "%.3f");
        if (changed)
        {
            v.x = a[0];
            v.y = a[1];
            v.z = a[2];
        }
        return changed;
    }
}
