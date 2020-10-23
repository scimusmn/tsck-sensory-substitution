// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function Settings(type)
{
    this.type = type;
    this.hue = { min: 0, max: 179 };
    this.sat = { min: 0, max: 255 };
    this.val = { min: 0, max: 255 };
    this.erosions = 0;
    this.dilations = 0;
}

Settings.prototype.updateUi = function()
{
    this.freeze = true;
    $('#hueRange').val(`${this.hue.min};${this.hue.max}`).trigger('change');
    $('#satRange').jRange('setValue',`${this.sat.min},${this.sat.max}`);
    $('#valRange').jRange('setValue',`${this.val.min},${this.val.max}`);
    $('#erosions').val(this.erosions);
    $('#dilations').val(this.dilations);
    this.freeze = false;
}

Settings.prototype.updateFromUi = function()
{
    if (this.freeze)
	return;
    [ this.hue.min, this.hue.max ] = $('#hueRange').val().split(';');
    [ this.sat.min, this.sat.max ] = $('#satRange').val().split(',');
    [ this.val.min, this.val.max ] = $('#valRange').val().split(',');
    this.erosions = $('#erosions').val();
    this.dilations = $('#dilations').val();
}

Settings.prototype.get = function()
{
    $.get(`/get/${this.type}Settings`,
	  (data, success) => {
	      if (success !== 'success')
		  return;
	      data = JSON.parse(data);
	      console.log(data);

	      this.hue.min = data.hueMin;
	      this.hue.max = data.hueMax;

	      this.sat.min = data.satMin;
	      this.sat.max = data.satMax;

	      this.val.min = data.valMin;
	      this.val.max = data.valMax;

	      this.dilations = data.dilations;
	      this.erosions = data.erosions;
	      console.log(this);
	  });
}

Settings.prototype.send = function()
{
    let data = {
	callback: 'settings',
	type: this.type,
	hueMin: this.hue.min,
	satMin: this.sat.min, 
	valMin: this.val.min,
	hueMax: this.hue.max,
	satMax: this.sat.max,
	valMax: this.val.max,
	erosions: this.erosions,
	dilations: this.dilations,
    };

    $.post('/post', data);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function save() {
    data = { callback: 'save' };
    $.post('/post',data);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

$(document).ready(function() {
    let ballSettings = new Settings('ball');
    let handSettings = new Settings('hand');

    let currentSettings = ballSettings;
    
    $('input.nice-number').niceNumber();
    $('input.circle-range-select').lcnCircleRangeSelect();
    $('input.jquery-range').jRange({
	from: 0,
	to: 255,
	step: 1,
	isRange: true,
	theme: 'theme-config',
    });

    let update = function()
    {
	let type = $('#typeSelect').val();
	if (type === 'ball') {
	    ballSettings.updateFromUi();
	    ballSettings.send();
	}
	else if (type === 'hand') {
	    handSettings.updateFromUi();
	    handSettings.send();
	}
    };

    update();

    $('#typeSelect').on('change', () =>
	{
	    let type = $('#typeSelect').val();
	    if (type === 'ball')
		ballSettings.updateUi();
	    else if (type === 'hand')
		handSettings.updateUi();
	});
	    
    
    $('#hueRange').on('change', update);
    $('#satRange').on('change', update);
    $('#valRange').on('change', update);
    $('#erosions').siblings('button').on('click', update);
    $('#dilations').siblings('button').on('click', update);  

    ballSettings.get();
    handSettings.get();
    ballSettings.updateUi();

    /*window.setInterval( function() {
      $.get('/get/cameraImage',function(data, status) {
      $('#cameraImage').attr('src',`data:image/jpeg;base64,${data}`);
      });
      $.get('/get/ballMask',function(data, status) {
      $('#ballMaskImage').attr('src',`data:image/jpeg;base64,${data}`);
      });
      $.get('/get/bgMask',function(data, status) {
      $('#bgMaskImage').attr('src',`data:image/jpeg;base64,${data}`);
      });
      }, 100);*/
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
