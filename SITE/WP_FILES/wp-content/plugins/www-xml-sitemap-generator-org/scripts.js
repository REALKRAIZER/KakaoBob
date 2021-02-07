			function xsg_populate(id, options, selected)
			{
				var select = document.getElementById(id);
			
				for ( var i = 0; i < options.length; i++) 
				{
					var opt = options[i];
					var el = document.createElement("option");
					el.textContent = opt[1];
					el.value = opt[0];
					select.appendChild(el);
					if (opt[0] == selected) {
						select.selectedIndex = i;
					}
				}
				
			}
			
		
			var  excludeDefaults = [[2,"exclude"] , [3,"include"]    ]; 
			var  priorityDefaults = [[0,"none"], [2,"0.0"], [3,"0.1"], [4,"0.2"], [5,"0.3"], [6,"0.4"] , [7,"0.5"] , [8,"0.6"] , [9,"0.7"] , [10,"0.8"] , [11,"0.9"], [12,"1.0"]  ]; 
			var  frequencyDefaults = [[0,"none"],  [8,"always"], [7,"hourly"], [6,"daily"], [5,"weekly"], [4,"monthly"], [3,"yearly"] , [2,"never"] ]; 
			
			var  inheritSelect = [ [0,"Don't inherit"], [1,"Inherit"]  ]; 
			var  excludeSelect = [  [1,"default"], [2,"exclude"] , [3,"include"]  ]; 
			var  prioritySelect = [[0,"none"],[1, "default"], [2,"0.0"], [3,"0.1"], [4,"0.2"], [5,"0.3"], [6,"0.4"] , [7,"0.5"] , [8,"0.6"] , [9,"0.7"] , [10,"0.8"] , [11,"0.9"], [12,"1.0"]  ]; 
			var  frequencySelect =  [[0,"none"],[1, "default"], [8,"always"], [7,"hourly"],  [6,"daily"], [5,"weekly"], [4,"monthly"], [3,"yearly"] , [2,"never"] ];
			