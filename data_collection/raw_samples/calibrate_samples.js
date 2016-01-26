var fs = require('fs');

module.exports = {
	calculate_constants: function (wattage, dataArr) {
		var dataLen = dataArr.length / 2;

		var voltageArr = [dataLen];
		var currentArr = [dataLen];
		var voltageIndex = 0;
		var currentIndex = 1;
		fs.writeFileSync('rawSamples.dat', '# Count\tVoltage\tCurrent\tInt\n');
		for(var i = 0; i < dataLen; i++) {
			voltageArr[i] = dataArr[2*i + voltageIndex];
			if(voltageArr[i] > 127) {
				voltageArr[i] -= 256;
			}
			currentArr[i] = dataArr[2*i + currentIndex];
			if(currentArr[i] > 127) {
				currentArr[i] -= 256;
			}

			// if((i % ((504/2)-1)) == 0) {
			// 	voltageIndex ^= 1;
			// 	currentIndex ^= 1;
			// }
		}

		var voff = 0;
		var ioff = 0;
		for(var i = 0; i < dataLen; i++) {
			voff += voltageArr[i];
			ioff += currentArr[i];
		}
		voff = Math.round(voff / dataLen);
		ioff = Math.round(ioff / dataLen);

		var integrate = [dataLen];

		var curoff = 0;
		var aggCurrent = 0;
		var tempCurrent = currentArr[0] - ioff;
		console.log(currentArr[0])
		console.log(aggCurrent + (tempCurrent + (tempCurrent >> 1)));
		for(var i = 1; i < dataLen; i++) {
			var newCurrent = currentArr[i] - ioff;
			//console.log("Current = " + currentArr[i] + ", newCurrent = " + newCurrent);
			aggCurrent += (newCurrent + (newCurrent >> 1));
			aggCurrent -= aggCurrent >> 5;
			integrate[i] = aggCurrent;
			curoff += aggCurrent;

			//console.log(i + '\t' + voltageArr[i] + '\t' + currentArr[i] + '\t' + integrate[i]);
			fs.appendFileSync('rawSamples.dat', i + '\t' + voltageArr[i] + '\t' + currentArr[i] + '\t' + integrate[i] + '\n');
		}
		curoff = Math.round(curoff / dataLen);

		var sampleCount = 0;
		var acc_i_rms = 0;
		var acc_v_rms = 0;
		var acc_p_ave = 0;
		var wattHoursAve = 0;
		var voltAmpAve = 0;
		fs.writeFileSync('goodSamples.dat', '# Count\tVoltage\tCurrent\n');
		for(var i = 0; i < dataLen; i++) {
			var newVoltage = voltageArr[i] - voff;
			var newIntegrate = integrate[i] - curoff;
			acc_i_rms += newIntegrate * newIntegrate;
			acc_v_rms += newVoltage * newVoltage;
			acc_p_ave += newVoltage * newIntegrate;

			fs.appendFileSync('goodSamples.dat', i + '\t' + newVoltage + '\t' + newIntegrate + '\n');

			sampleCount++;
			if(sampleCount == 42) {
				sampleCount = 0;
				wattHoursAve += acc_p_ave / 42;
				console.log(wattHoursAve);
				acc_p_ave = 0;
				voltAmpAve += Math.sqrt(acc_v_rms / 42) * Math.sqrt(acc_i_rms / 42);
				acc_v_rms = 0;
				acc_i_rms = 0;
			}
		}

		var truePower = wattHoursAve / 60;
		console.log("True Power = " + truePower);
		var pscale_num = wattage / truePower;

		var pscale_val = 0x4000 + Math.floor(pscale_num*Math.pow(10,4));

		console.log("Ioff = " + ioff);
		console.log("Voff = " + voff);
		console.log("Curoff = " + curoff);
		console.log("Pscale = " + pscale_num);
		console.log("Pscale = " + pscale_val);

	    return {voff, ioff, curoff, pscale_val};
	}
}