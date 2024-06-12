window.onload = function () {
  slideOne();
  slideTwo();
  slideThree();
  slideFour();
  slideFive();
  slideSix();
  slideSeven();
  slideEight();
  slideNine();
  slideTen();
}

//slide one
let sliderOne = document.getElementById("slider-1");
let sliderTwo = document.getElementById("slider-2");
let displayValOne = document.getElementById("range1");
let displayValTwo = document.getElementById("range2");
let minGap = 0;
let sliderTrack = document.querySelector(".slider-track");
let sliderOneMaxValue = document.getElementById("slider-1").max;

function slideOne() {
  if (parseInt(sliderTwo.value) - parseInt(sliderOne.value) <= minGap) {
    sliderOne.value = parseInt(sliderTwo.value) - minGap;
  }
  displayValOne.textContent = sliderOne.value;
  fillColor1();
}
function slideTwo() {
  if (parseInt(sliderTwo.value) - parseInt(sliderOne.value) <= minGap) {
    sliderTwo.value = parseInt(sliderOne.value) + minGap;
  }
  displayValTwo.textContent = sliderTwo.value;
  fillColor1();
}
function fillColor1() {
  percent1 = (sliderOne.value / sliderOneMaxValue) * 100;
  percent2 = (sliderTwo.value / sliderOneMaxValue) * 100;
  sliderTrack.style.background = `linear-gradient(to right, #dadae5 ${percent1}% , #3264fe ${percent1}% , #3264fe ${percent2}%, #dadae5 ${percent2}%)`;
}

//slide two
let sliderThree = document.getElementById("slider-3");
let sliderFour = document.getElementById("slider-4");
let displayValThree = document.getElementById("range3");
let displayValFour = document.getElementById("range4");
let sliderTrack2 = document.querySelector(".slider-track2");
let sliderThreeMaxValue = document.getElementById("slider-3").max;

function slideThree() {
  if (parseInt(sliderFour.value) - parseInt(sliderThree.value) <= minGap) {
    sliderThree.value = parseInt(sliderFour.value) - minGap;
  }
  displayValThree.textContent = sliderThree.value;
  fillColor2();
}
function slideFour() {
  if (parseInt(sliderFour.value) - parseInt(sliderThree.value) <= minGap) {
    sliderFour.value = parseInt(sliderThree.value) + minGap;
  }
  displayValFour.textContent = sliderFour.value;
  fillColor2();
}
function fillColor2() {
  percent1 = (sliderThree.value / sliderThreeMaxValue) * 100;
  percent2 = (sliderFour.value / sliderThreeMaxValue) * 100;
  sliderTrack2.style.background = `linear-gradient(to right, #dadae5 ${percent1}% , #3264fe ${percent1}% , #3264fe ${percent2}%, #dadae5 ${percent2}%)`;
}

//slide three
let sliderFive = document.getElementById("slider-5");
let sliderSix = document.getElementById("slider-6");
let displayValFive = document.getElementById("range5");
let displayValSix = document.getElementById("range6");
let sliderTrack3 = document.querySelector(".slider-track3");
let sliderFiveMaxValue = document.getElementById("slider-5").max;

function slideFive() {
  if (parseInt(sliderSix.value) - parseInt(sliderFive.value) <= minGap) {
    sliderFive.value = parseInt(sliderSix.value) - minGap;
  }
  displayValFive.textContent = sliderFive.value;
  fillColor3();
}
function slideSix() {
  if (parseInt(sliderSix.value) - parseInt(sliderFive.value) <= minGap) {
    sliderSix.value = parseInt(sliderFive.value) + minGap;
  }
  displayValSix.textContent = sliderSix.value;
  fillColor3();
}
function fillColor3() {
  percent1 = (sliderFive.value / sliderFiveMaxValue) * 100;
  percent2 = (sliderSix.value / sliderFiveMaxValue) * 100;
  sliderTrack3.style.background = `linear-gradient(to right, #dadae5 ${percent1}% , #3264fe ${percent1}% , #3264fe ${percent2}%, #dadae5 ${percent2}%)`;
}

//slide four
let sliderSeven = document.getElementById("slider-7");
let sliderEight = document.getElementById("slider-8");
let displayValSeven = document.getElementById("range7");
let displayValEight = document.getElementById("range8");
let sliderTrack4 = document.querySelector(".slider-track4");
let sliderSevenMaxValue = document.getElementById("slider-7").max;

function slideSeven() {
  if (parseInt(sliderEight.value) - parseInt(sliderSeven.value) <= minGap) {
    sliderSeven.value = parseInt(sliderEight.value) - minGap;
  }
  displayValSeven.textContent = sliderSeven.value;
  fillColor4();
}
function slideEight() {
  if (parseInt(sliderEight.value) - parseInt(sliderSeven.value) <= minGap) {
    sliderEight.value = parseInt(sliderSeven.value) + minGap;
  }
  displayValEight.textContent = sliderEight.value;
  fillColor4();
}
function fillColor4() {
  percent1 = (sliderSeven.value / sliderSevenMaxValue) * 100;
  percent2 = (sliderEight.value / sliderSevenMaxValue) * 100;
  sliderTrack4.style.background = `linear-gradient(to right, #dadae5 ${percent1}% , #3264fe ${percent1}% , #3264fe ${percent2}%, #dadae5 ${percent2}%)`;
}

//slide five
let sliderNine = document.getElementById("slider-9");
let sliderTen = document.getElementById("slider-10");
let displayValNine = document.getElementById("range9");
let displayValTen = document.getElementById("range10");
let sliderTrack5 = document.querySelector(".slider-track5");
let sliderNineMaxValue = document.getElementById("slider-9").max;

function slideNine() {
  if (parseInt(sliderTen.value) - parseInt(sliderNine.value) <= minGap) {
    sliderNine.value = parseInt(sliderTen.value) - minGap;
  }
  displayValNine.textContent = sliderNine.value;
  fillColor5();
}
function slideTen() {
  if (parseInt(sliderTen.value) - parseInt(sliderNine.value) <= minGap) {
    sliderTen.value = parseInt(sliderNine.value) + minGap;
  }
  displayValTen.textContent = sliderTen.value;
  fillColor5();
}
function fillColor5() {
  percent1 = (sliderNine.value / sliderNineMaxValue) * 100;
  percent2 = (sliderTen.value / sliderNineMaxValue) * 100;
  sliderTrack5.style.background = `linear-gradient(to right, #dadae5 ${percent1}% , #3264fe ${percent1}% , #3264fe ${percent2}%, #dadae5 ${percent2}%)`;
}
