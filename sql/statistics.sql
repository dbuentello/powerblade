# Deployment stats
alter view inf_dep_stats as
select t1.location, case when t3.gndTruth is not null then 1 else 0 end as gndTruth, greatest(t1.startTime, t2.startTime) as startTime, least(t1.endTime, t2.endTime) as endTime, least(t1.duration, t2.duration) as duration, t1.pb_count, t2.gw_count from
inf_dep_pb t1
join inf_dep_gw t2
on t1.location=t2.location
left join inf_gnd_truth_lookup t3
on t1.location=t3.location
order by location;

create view inf_dep_pb as
select location, min(startTime) as startTime, least(utc_date(), max(endTime)) as endTime, 
1+datediff(least(utc_date(), max(endTime)), min(startTime)) as duration,
count(*) as pb_count 
from most_recent_powerblades 
where location!=10
group by location;

select * from inf_dep_pb;

create view inf_dep_gw as
select location, min(startTime) as startTime, least(utc_date(), max(endTime)) as endTime,
1+datediff(least(utc_date(), max(endTime)), min(startTime)) as duration,
count(*) as gw_count
from most_recent_gateways
where location!=10
group by location;

create view inf_gnd_truth_lookup as
select location,1 as gndTruth from dat_gnd_truth group by location;

select * from inf_dep_stats;

select count(*) as numDeps, avg(duration) as avgDuration, stddev(duration) as stdDuration, min(duration) as minDuration, max(duration) as maxDuration, 
sum(pb_count) as sumPb, avg(pb_count) as  avgPb, stddev(pb_count) as stdPb, min(pb_count) as minPb, max(pb_count) as maxPb,
sum(gw_count) as sumGw, avg(gw_count) as avgGw, min(gw_count) as minGw, max(gw_count) as maxGw 
from inf_dep_stats;
where duration > 7;
 and duration < 168;

select * from most_recent_powerblades where location=3;

select * from inf_pb_lookup where deviceMAC='c098e57000cd';
select * from most_recent_powerblades where deviceMAC='c098e57000cd';
select * from active_powerblades;# where deviceMAC='c098e57000cd';

select gatewayMAC, count(distinct(location)) from inf_gw_lookup group by gatewayMAC;
select * from inf_gw_lookup where gatewayMAC='c098e5c00026';
select * from most_recent_gateways order by location;
select * from active_gateways order by location;


# This is for determining if a PowerBlade has been redeployed (IMPORTANT!)
select t1.*, t2.count from
most_recent_powerblades t1 join
(select deviceMAC, count(distinct(deviceName)) as count from most_recent_powerblades group by deviceMAC order by count desc) t2
on t1.deviceMAC=t2.deviceMAC
order by t2.count desc, t1.deviceMAC asc, t1.startTime asc;

select deviceMAC, max(power) as maxPower from dat_powerblade force index (devTimePower)
where timestamp>='2017-3-27  00:00:00'# and timestamp<='2017-3-27  23:59:59' 
and power != 120.13 
and deviceMAC in ("c098e57000f6","c098e57000d9")#,"c098e57000e8","c098e57000d5","c098e57000fb","c098e57000bf","c098e57000e3","c098e570006b","c098e57000ea","c098e57000f9","c098e57000e1","c098e57000e9","c098e57000d1","c098e57000eb","c098e57000cd","c098e57000d8","c098e5700115","c098e57000ee","c098e57000ed","c098e57000d6","c098e57000ce","c098e57000ec","c098e57000cf","c098e57000f4","c098e57000c0","c098e5700100","c098e57000f3") 
group by deviceMAC;

select date(timestamp) as dayst, deviceMAC, (max(energy) - min(energy)) as dayEnergy from dat_powerblade force index (devTimePower) 
where timestamp>='2017-3-25  00:00:00' and timestamp<='2017-3-27  23:59:59' 
and deviceMAC in ("c098e570015e","c098e57001b1","c098e570019f","c098e57001c4","c098e57001f5","c098e570015e","c098e57001b6","c098e57001c3","c098e5700197","c098e57001b9","c098e57001c0","c098e57001c9","c098e57001be","c098e5700286","c098e570027d","c098e57001b0","c098e570027f","c098e5700284","c098e57001b2","c098e5700190","c098e570019d","c098e570005d","c098e57001bb","c098e57001c2","c098e57001f4","c098e57001b8","c098e57001c1","c098e5700198","c098e57001bd","c098e57001c7","c098e57001f6","c098e57001b3","c098e570027e","c098e5700285","c098e57001af","c098e570027b","c098e5700280","c098e570019b","c098e57001ba","c098e57001c5","c098e57001ed","c098e57001b7","c098e57001c8","c098e57001ee","c098e57001bc","c098e57001c6","c098e57001f3","c098e57001bf","c098e5700283","c098e5700281","c098e57001b4","c098e5700282","c098e570027c") 
and energy!=999999.99 
group by deviceMAC, dayst;

select deviceMAC, timestamp, min(power) from dat_powerblade force index (devPower) group by timestamp, deviceMAC;

select * from dat_powerblade where timestamp<'2017-01-13' order by timestamp desc limit 1;

describe inf_pb_lookup;


select * from dat_powerblade where deviceMAC='c098e5700139' order by id asc limit 100;
select * from dat_powerblade where gatewayMAC in ('c098e5c00029', 'c098e5c00029') and deviceMAC='c098e570005b' order by id desc;



'alter view maxPower_pb as ' \
		'select deviceMAC, max(power) as maxPower from dat_powerblade force index (devPower) ' \
		'where timestamp>=\'' + config['startDay'] + ' 00:00:00\' and timestamp<=\'' + config['endDay'] + ' 23:59:59\' ' \
		'and power != 120.13 ' \
		'and deviceMAC in ' + dev_powerblade + ' group by deviceMAC;'
        
select deviceMAC, max(power) as maxPower from dat_powerblade force index (devPower)
where timestamp>(select startTime from ;

select deviceMAC, count(*) as count from most_recent_powerblades group by deviceMAC order by count desc;

select * from most_recent_powerblades where deviceMAC='c098e57000f3';


select * from dat_powerblade force index (devEnergy)
where timestamp<'2017-03-26 23:59:59'
and deviceMAC='c098e57000f3';


select deviceMAC, max(power) as maxPower from dat_powerblade force index (devTimePower) 
where timestamp>='2017-1-13  00:00:00' and timestamp<='2017-3-27  23:59:59' 
and power != 120.13 
and deviceMAC in ("c098e57000f6","c098e57000d9","c098e57000e8","c098e57000d5","c098e57000fb","c098e57000bf","c098e57000e3","c098e570006b","c098e57000ea","c098e57000f9","c098e57000e1","c098e57000e9","c098e57000d1","c098e57000eb","c098e57000cd","c098e57000d8","c098e5700115","c098e57000ee","c098e57000ed","c098e57000d6","c098e57000ce","c098e57000ec","c098e57000cf","c098e57000f4","c098e57000c0","c098e5700100","c098e57000f3") 
group by deviceMAC;

select * from perm_maxPower_pb;
select * from mr_maxPower_pb;
select * from perm_avgPower_pb;
select * from mr_avgPower_pb;

insert into perm_maxPower_pb (deviceMAC, maxPower)
(select deviceMAC, max(power) as maxPower from dat_powerblade force index (devTimePower) 
where timestamp>='2017-1-13  00:00:00' and timestamp<='2017-3-27  23:59:59' 
and power != 120.13 
and deviceMAC in ("c098e57000f6","c098e57000d9","c098e57000e8","c098e57000d5","c098e57000fb","c098e57000bf","c098e57000e3","c098e570006b","c098e57000ea","c098e57000f9","c098e57000e1","c098e57000e9","c098e57000d1","c098e57000eb","c098e57000cd","c098e57000d8","c098e5700115","c098e57000ee","c098e57000ed","c098e57000d6","c098e57000ce","c098e57000ec","c098e57000cf","c098e57000f4","c098e57000c0","c098e5700100","c098e57000f3") 
group by deviceMAC);

select deviceMAC, max(power) as maxPower from dat_powerblade force index (devTimePower) 
where timestamp>='2017-1-13  00:00:00' and timestamp<='2017-3-27  23:59:59' 
and power != 120.13 
and deviceMAC in ("c098e57000f6","c098e57000d9","c098e57000e8","c098e57000d5","c098e57000fb","c098e57000bf","c098e57000e3","c098e570006b","c098e57000ea","c098e57000f9","c098e57000e1","c098e57000e9","c098e57000d1","c098e57000eb","c098e57000cd","c098e57000d8","c098e5700115","c098e57000ee","c098e57000ed","c098e57000d6","c098e57000ce","c098e57000ec","c098e57000cf","c098e57000f4","c098e57000c0","c098e5700100","c098e57000f3") 
group by deviceMAC;


select deviceMAC, avg(power) as avgPower from dat_powerblade t1 force index(devTimePower) 
where timestamp>='2017-3-01  00:00:00' and timestamp<='2017-3-27  23:59:59' 
and power>(select 0.1*maxPower from mr_maxPower_pb t2 where t1.deviceMAC=t2.deviceMAC) 
and deviceMAC in ("c098e57000f6","c098e57000d9","c098e57000e8","c098e57000d5","c098e57000fb","c098e57000bf","c098e57000e3","c098e570006b","c098e57000ea","c098e57000f9","c098e57000e1","c098e57000e9","c098e57000d1","c098e57000eb","c098e57000cd","c098e57000d8","c098e5700115","c098e57000ee","c098e57000ed","c098e57000d6","c098e57000ce","c098e57000ec","c098e57000cf","c098e57000f4","c098e57000c0","c098e5700100","c098e57000f3") 
group by deviceMAC;











