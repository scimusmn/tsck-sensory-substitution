// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

let ballSettings = {};
let bgSettings   = {};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function updateBallSettings() {
  let [ hueMin, hueMax ] = $('#ballHueRange').val().split(';');
  let [ satMin, satMax ] = $('#ballSatRange').val().split(',');
  let [ valMin, valMax ] = $('#ballValRange').val().split(',');
  let erosions = $('#ballErosions').val();
  let dilations = $('#ballDilations').val();

  let data = {
    callback: 'setBallSettings',
    hueMin,
    hueMax,
    satMin,
    satMax,
    valMin,
    valMax,
    erosions,
    dilations,
  };
  $.post('/post', data);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

function updateBgSettings() {
  let [ hueMin, hueMax ] = $('#bgHueRange').val().split(';');
  let [ satMin, satMax ] = $('#bgSatRange').val().split(',');
  let [ valMin, valMax ] = $('#bgValRange').val().split(',');
  let erosions = $('#bgErosions').val();
  let dilations = $('#bgDilations').val();

  let data = {
    callback: 'setBgSettings',
    hueMin,
    hueMax,
    satMin,
    satMax,
    valMin,
    valMax,
    erosions,
    dilations,
  };
  $.post('/post', data);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

$(document).ready(function() {
  $.get('/get/ballSettings', function(data, status) {
    if (status === 'success') {
      ballSettings = JSON.parse(data);
      $('#ballHueRange').val(`${ballSettings.hueMin};${ballSettings.hueMax}`).trigger('change');
      $('#ballSatRange').jRange('setValue',`${ballSettings.satMin},${ballSettings.satMax}`);
      $('#ballValRange').jRange('setValue',`${ballSettings.valMin},${ballSettings.valMax}`);
      $('#ballErosions').val(ballSettings.erosions);
      $('#ballDilations').val(ballSettings.dilations);            
    }
  });

  $.get('/get/bgSettings', function(data, status) {
    if (status === 'success') {
      bgSettings = JSON.parse(data);
      $('#bgHueRange').val(`${bgSettings.hueMin};${bgSettings.hueMax}`).trigger('change');
      $('#bgSatRange').jRange('setValue',`${bgSettings.satMin},${bgSettings.satMax}`);
      $('#bgValRange').jRange('setValue',`${bgSettings.valMin},${bgSettings.valMax}`);
      $('#bgErosions').val(bgSettings.erosions);
      $('#bgDilations').val(bgSettings.dilations);
    }
  });

  $('input.nice-number').niceNumber();
  $('input.circle-range-select').lcnCircleRangeSelect();
  $('input.jquery-range').jRange({
    from: 0,
    to: 255,
    step: 1,
    isRange: true,
    theme: 'theme-config',
  });

  $('#ballHueRange').on('change',updateBallSettings);
  $('#ballSatRange').on('change',updateBallSettings);
  $('#ballValRange').on('change',updateBallSettings);

  $('#bgHueRange').on('change',updateBgSettings);
  $('#bgSatRange').on('change',updateBgSettings);
  $('#bgValRange').on('change',updateBgSettings);
  

  window.setInterval( function() {
    $.get('/get/cameraImage',function(data, status) {
      $('#cameraImage').attr('src',`data:image/jpeg;base64,${data}`);
    });
    $.get('/get/ballMask',function(data, status) {
      $('#ballMaskImage').attr('src',`data:image/jpeg;base64,${data}`);
    });
    $.get('/get/bgMask',function(data, status) {
      $('#bgMaskImage').attr('src',`data:image/jpeg;base64,${data}`);
    });
  }, 100);
});

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
