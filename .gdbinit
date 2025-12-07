
define paging_off
    maintenance packet Qqemu.PhyMemMode:1
end

define paging_on
    maintenance packet Qqemu.PhyMemMode:0
end
