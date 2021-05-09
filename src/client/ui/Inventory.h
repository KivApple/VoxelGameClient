#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>
#include <mutex>
#include "UIBase.h"
#include "world/Voxel.h"

class InventoryItem: public UIElement {
	static const float FILL_BUFFER_DATA[];
	static const float OUTLINE_BUFFER_DATA[];
	
	VoxelHolder m_voxel;
	GL::Texture m_texture;
	bool m_active = false;
	
	void updateTexture();
	
public:
	InventoryItem();
	void render(const glm::mat4 &transform) override;
	[[nodiscard]] const VoxelHolder &voxel() const {
		return m_voxel;
	}
	void setVoxel(const VoxelHolder &voxel);
	void setVoxel(VoxelHolder &&voxel);
	[[nodiscard]] bool active() const {
		return m_active;
	}
	void setActive(bool active);
	
};

class Inventory: public UIElementGroup {
	int m_cols;
	int m_rows;
	int m_activeIndex = -1;
	std::vector<std::unique_ptr<InventoryItem>> m_items;
	std::unordered_map<uint8_t, VoxelHolder> m_nextChanges;
	int m_nextActive = -1;
	bool m_hasUpdate = false;
	std::mutex m_mutex;
	
public:
	Inventory(int cols, int rows);
	void render(const glm::mat4 &transform) override;
	[[nodiscard]] int size() const {
		return m_cols * m_rows;
	}
	[[nodiscard]] const InventoryItem &item(int index) const {
		return *m_items[index];
	}
	[[nodiscard]] const InventoryItem &item(int col, int row) const;
	[[nodiscard]] InventoryItem &item(int index) {
		return *m_items[index];
	}
	InventoryItem &item(int col, int row);
	[[nodiscard]] int activeIndex() const {
		return m_activeIndex;
	}
	void setActive(int index);
	void setActive(int col, int row) {
		setActive(col + row * m_cols);
	}
	void setUpdate(std::unordered_map<uint8_t, VoxelHolder> &&changes, int active);
	void update();
	
};
