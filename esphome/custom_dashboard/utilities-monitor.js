document.addEventListener('DOMContentLoaded', function () {
  const tabContainer = document.createElement('div');
  tabContainer.className = 'tab';
  document.body.insertBefore(tabContainer, document.body.firstChild);

  const groups = [
    { id: 'group_network_info', name: 'Network Information' },
    { id: 'group_environmental_sensors', name: 'Environmental Sensors' },
    { id: 'group_water_tank_sensors', name: 'Water Tank Sensors' },
    { id: 'group_gas_mj_sensors', name: 'Gas Sensors (MJ)' },
    { id: 'group_gas_m3_sensors', name: 'Gas Sensors (m³)' },
    { id: 'group_town_water_sensors', name: 'Town Water Sensors' },
    { id: 'group_utility_meter_sensors', name: 'Gas & Water Meter Sensors' },
  ];

  groups.forEach(group => {
    const btn = document.createElement('button');
    btn.textContent = group.name;
    btn.onclick = () => showGroup(group.id);
    tabContainer.appendChild(btn);
  });

  function showGroup(groupId) {
    document.querySelectorAll('.esphome-group').forEach(el => {
      el.style.display = 'none';
    });
    document.querySelector(`[data-group-id="${groupId}"]`).style.display = 'block';

    document.querySelectorAll('.tab button').forEach(btn => btn.classList.remove('active'));
    [...tabContainer.children].find(btn => btn.textContent === groups.find(g => g.id === groupId).name).classList.add('active');
  }

  // Assign data-group-id for proper toggling
  groups.forEach(group => {
    const groupEl = document.querySelector(`.esphome-group:has(h2:contains("${group.name}"))`);
    if (groupEl) {
      groupEl.dataset.groupId = group.id;
    }
  });

  // Auto select first tab
  if (groups.length > 0) showGroup(groups[0].id);

  // Logs positioning
  const logArea = document.querySelector('#log-output');
  if (logArea) {
    logArea.id = 'logs';
  }
});
