all:
	scp rennsemmel:/home/jrs/rgbsensortest_data.sqlite3 .
	python3 plotdata.py
