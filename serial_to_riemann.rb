#!/usr/bin/ruby

require 'rubygems'
require 'riemann/client'

riemann ||= Riemann::Client.new(
        :host => '127.0.0.1',
        :port => 5555
      )

def get_state(temp)
	if temp >= 28
		state = :hot
	elsif temp >= 25
		state = :warm
	elsif temp >= 21
		state = :ok
	else
		state = :cold
	end
end

areas = Hash.new

line_num=0
File.open('/dev/ttyUSB0').each do |line|
	print "#{line_num += 1} #{line}"
	parts = line.split(' ')
	next unless parts.first and parts.first == 'OK'
	id = parts[1]
	temp = parts[2].to_f

	areas[id] = 0 unless areas[id]

	areas[id] = areas[id] + 1

	event = {
		:host => "environment - #{id}",
		:service => 'temperature',
		:metric => temp,
		:ttl => 300,
		:state => get_state(temp).to_s
	}
	puts event.inspect
	riemann << event

	puts areas.inspect
end
